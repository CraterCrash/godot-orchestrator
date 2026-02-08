// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef ORCHESTRATOR_CALLABLE_LAMBDA_H
#define ORCHESTRATOR_CALLABLE_LAMBDA_H

#include <utility>

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_custom.hpp>

namespace callable_internal {

    using namespace godot;

    template<typename T>
    struct function_traits : public function_traits<decltype(&T::operator())> {
    };

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const> {
        using result_type = ReturnType;
        using arg_tuple = std::tuple<Args...>;
        static constexpr auto arity = sizeof...(Args);
    };

    template<class T>
    struct ExpandPack;

    template<class ...Args>
    struct ExpandPack<std::tuple<Args...>> {
        template<class L, std::size_t ...Is>
        _FORCE_INLINE_ static void call(L&& l, const Variant **p_arguments, int p_argcount, Variant &r_return_value,
                                        GDExtensionCallError &r_call_error, IndexSequence<Is...>) {
            #ifdef DEBUG_ENABLED
            if ((size_t)p_argcount > sizeof...(Args)) {
                r_call_error.error = GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;
                r_call_error.expected = (int32_t)sizeof...(Args);
                return;
            }

            if ((size_t)p_argcount < sizeof...(Args)) {
                r_call_error.error = GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS;
                r_call_error.expected = (int32_t)sizeof...(Args);
                return;
            }
            #endif

            r_call_error.error = GDEXTENSION_CALL_OK;

            #ifdef DEBUG_METHODS_ENABLED
            l(VariantCasterAndValidate<Args>::cast(p_arguments, Is, r_call_error)...);
            #else
            l(VariantCaster<Args>::cast(*p_arguments[Is])...);
            #endif
            (void)(p_arguments); // Avoid warning.
        }
    };

    template<class Lambda>
    class CallableCustomLambda : public CallableCustom {
        Lambda _lambda;
        Object* _instance;

    public:
        //~ Begin CallableCustom Interface
        uint32_t hash() const override { return (intptr_t) this; }
        String get_as_text() const override { return "CallableCustomLambda"; }
        CompareEqualFunc get_compare_equal_func() const override { return [](const CallableCustom* a, const CallableCustom* b) { return a->hash() == b->hash(); }; }
        CompareLessFunc get_compare_less_func() const override { return [](const CallableCustom* a, const CallableCustom* b) { return a->hash() < b->hash(); }; }
        bool is_valid() const override { return _instance != nullptr; }
        ObjectID get_object() const override { return ObjectID(); }
        void call(const Variant** p_args, int p_arg_count, Variant& r_ret, GDExtensionCallError& r_error) const override {
            using traits = function_traits<Lambda>;
            using E = ExpandPack<typename traits::arg_tuple>;
            E::call(_lambda, p_args, p_arg_count, r_ret, r_error, BuildIndexSequence<traits::arity>{});
        }
        //~ End CallableCustom Interface

        /// Constructor
        /// @param p_instance the instance the callable is invoked upon
        /// @param p_lambda the lambda function to call
        CallableCustomLambda(Object* p_instance, Lambda&& p_lambda)
            : _lambda(p_lambda)
            , _instance(p_instance) {
        }
    };
}

template<class Lambda>
godot::Callable callable_mp_lambda(godot::Object* p_instance, Lambda&& p_lambda) {
    auto* ccl = memnew(callable_internal::CallableCustomLambda(p_instance, std::forward<Lambda>(p_lambda)));
    return godot::Callable(ccl);
}

#define callable_mp_signal_lambda(name, ...) callable_mp_lambda(this, [this] { emit_signal(name __VA_ARGS__ ); })

#endif