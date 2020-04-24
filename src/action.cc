/**
 * action.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/action.h"

#include <exception>  // std::runtime_error, std::invalid_argument
#include <cassert>    // assert
#include <map>        // std::map
#include <sstream>    // std::stringstream

#include "symbolic/pddl.h"
#include "symbolic/utils/parameter_generator.h"

namespace {

using ::symbolic::Object;
using ::symbolic::Proposition;
using ::symbolic::Pddl;
using ::symbolic::State;

using ActionFunction = std::function<State(const State&, const std::vector<Object>&)>;

using EffectsFunction = std::function<bool(const std::vector<Object>&, State*)>;

using ApplicationFunction = std::function<std::vector<Object>(const std::vector<Object>&)>;

EffectsFunction CreateEffectsFunction(const Pddl& pddl, const VAL::effect_lists* effects,
                                      const std::vector<Object>& parameters);

EffectsFunction CreateForall(const Pddl& pddl, const VAL::forall_effect* effect,
                             const std::vector<Object>& parameters) {
  // Create forall parameters
  std::vector<Object> forall_params = parameters;
  const std::vector<Object> types = symbolic::ConvertObjects(pddl, effect->getVarsList());
  forall_params.insert(forall_params.end(), types.begin(), types.end());
  EffectsFunction ForallEffects = CreateEffectsFunction(pddl, effect->getEffects(), forall_params);

  return [&pddl, gen = symbolic::ParameterGenerator(pddl.object_map(), types),
          ForallEffects = std::move(ForallEffects)](const std::vector<Object>& arguments,
                                                    State* state) {
    // Loop over forall arguments
    bool is_state_changed = false;
    for (const std::vector<Object>& forall_objs : gen) {
      // Create forall arguments
      std::vector<Object> forall_args = arguments;
      forall_args.insert(forall_args.end(), forall_objs.begin(), forall_objs.end());

      is_state_changed |= ForallEffects(forall_args, state);
    }
    return is_state_changed;
  };
}

EffectsFunction CreateAdd(const Pddl& pddl, const VAL::simple_effect* effect,
                          const std::vector<Object>& parameters) {
  // Prepare effect argument application functions
  const std::vector<Object> effect_params = symbolic::ConvertObjects(pddl, effect->prop->args);
  ApplicationFunction Apply = symbolic::CreateApplicationFunction(parameters, effect_params);

  return [name_predicate = effect->prop->head->getName(),
          Apply = std::move(Apply)](const std::vector<Object>& arguments, State* state) {
    return state->emplace(name_predicate, Apply(arguments));
  };
}

EffectsFunction CreateDel(const Pddl& pddl, const VAL::simple_effect* effect,
                          const std::vector<Object>& parameters) {
  // Prepare effect argument application functions
  const std::vector<Object> effect_params = symbolic::ConvertObjects(pddl, effect->prop->args);
  ApplicationFunction Apply = symbolic::CreateApplicationFunction(parameters, effect_params);

  return [name_predicate = effect->prop->head->getName(),
          Apply = std::move(Apply)](const std::vector<Object>& arguments, State* state) {
    return state->erase(Proposition(name_predicate, Apply(arguments)));
  };
}

EffectsFunction CreateCond(const Pddl& pddl, const VAL::cond_effect* effect,
                           const std::vector<Object>& parameters) {
  const symbolic::Formula Condition(pddl, effect->getCondition(), parameters);
  EffectsFunction CondEffects = CreateEffectsFunction(pddl, effect->getEffects(), parameters);
  return [Condition = std::move(Condition),
          CondEffects = std::move(CondEffects)](const std::vector<Object>& arguments,
                                                State* state) {
    // TODO: Condition might return different results depending on ordering of
    // other effects since state is modified in place.
    if (Condition(*state, arguments)) {
      return CondEffects(arguments, state);
    }
    return false;
  };
}

EffectsFunction CreateEffectsFunction(const Pddl& pddl, const VAL::effect_lists* effects,
                                      const std::vector<Object>& parameters) {
  std::vector<EffectsFunction> effect_functions;
  // Forall effects
  for (const VAL::forall_effect* effect : effects->forall_effects) {
    effect_functions.emplace_back(CreateForall(pddl, effect, parameters));
  }

  // Add effects
  for (const VAL::simple_effect* effect : effects->add_effects) {
    effect_functions.emplace_back(CreateAdd(pddl, effect, parameters));
  }

  // Del effects
  for (const VAL::simple_effect* effect : effects->del_effects) {
    effect_functions.emplace_back(CreateDel(pddl, effect, parameters));
  }

  // Cond effects
  for (const VAL::cond_effect* effect : effects->cond_effects) {
    effect_functions.emplace_back(CreateCond(pddl, effect, parameters));
  }

  return [effect_functions = std::move(effect_functions)](const std::vector<Object>& arguments,
                                                          State* state) {
    bool is_state_changed = false;
    for (const EffectsFunction& Effect : effect_functions) {
      is_state_changed |= Effect(arguments, state);
    }
    return is_state_changed;
  };
}

const VAL::operator_* GetSymbol(const Pddl& pddl, const std::string& name_action) {
  assert(pddl.domain().ops != nullptr);
  for (const VAL::operator_* op : *pddl.domain().ops) {
    assert(op != nullptr && op->name != nullptr);
    if (op->name->getName() == name_action) return op;
  }
  throw std::runtime_error("Action::Action(): Could not find action symbol " + name_action + ".");
  return nullptr;
}

}  // namespace

namespace symbolic {

Action::Action(const Pddl& pddl, const VAL::operator_* symbol)
    : symbol_(symbol),
      name_(symbol_->name->getName()),
      parameters_(ConvertObjects(pddl, symbol_->parameters)),
      param_gen_(pddl.object_map(), parameters_),
      Preconditions_(pddl, symbol_->precondition, parameters_),
      Apply_(CreateEffectsFunction(pddl, symbol_->effects, parameters_)) {}

Action::Action(const Pddl& pddl, const std::string& action_call)
    : symbol_(GetSymbol(pddl, ParseHead(action_call))),
      name_(symbol_->name->getName()),
      parameters_(ConvertObjects(pddl, symbol_->parameters)),
      param_gen_(pddl.object_map(), parameters_),
      Preconditions_(pddl, symbol_->precondition, parameters_),
      Apply_(CreateEffectsFunction(pddl, symbol_->effects, parameters_)) {}

State Action::Apply(const State& state, const std::vector<Object>& arguments) const {
  State next_state(state);
  Apply_(arguments, &next_state);
  return next_state;
}

std::string Action::to_string() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::string Action::to_string(const std::vector<Object>& arguments) const {
  std::stringstream ss;
  ss << name_ << "(";
  std::string separator;
  for (const Object& arg : arguments) {
    ss << separator << arg;
    if (separator.empty()) separator = ", ";
  }
  ss << ")";
  return ss.str();
}

std::pair<Action, std::vector<Object>> ParseAction(const Pddl& pddl, const std::string& action_call) {
  const auto aa = std::make_pair<Action, std::vector<Object>>(Action(pddl, action_call),
                                                              ParseArguments(pddl, action_call));
  const Action& action = aa.first;
  const std::vector<Object>& args = aa.second;

  // Check number of arguments
  if (action.parameters().size() != args.size()) {
    std::stringstream ss;
    ss << "symbolic::ParseAction(): action " << action
       << " requires " << action.parameters().size() << " arguments but received "
       << args.size() << ": " << action_call << ".";
    throw std::invalid_argument(ss.str());
  }

  // Check argument types
  for (size_t i = 0; i < action.parameters().size(); i++) {
    const Object& param = action.parameters()[i];
    const Object& arg = args[i];
    if (!arg.type().IsSubtype(param.type())) {
      std::stringstream ss;
      ss << "symbolic::ParseAction(): action " << action
         << " requires parameter " << param << " to be of type "
         << param.type() << " but received " << arg << " with type " << arg
         << ": " << action_call << ".";
      throw std::invalid_argument(ss.str());
    }
  }

  return aa;
}

std::ostream& operator<<(std::ostream& os, const Action& action) {
  os << action.name() << "(";
  std::string separator;
  for (const Object& param : action.parameters()) {
    os << separator << param;
    if (separator.empty()) separator = ", ";
  }
  os << ")";
  return os;
}

}  // namespace symbolic
