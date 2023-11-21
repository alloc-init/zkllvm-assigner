//---------------------------------------------------------------------------//
// Copyright (c) 2022 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2022 Nikita Kaskov <nbering@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_ASSIGNER_RECURSIVE_PROVER_LOOKUP_ARG_VERIFIER_HPP
#define CRYPTO3_ASSIGNER_RECURSIVE_PROVER_LOOKUP_ARG_VERIFIER_HPP

#include "llvm/IR/Type.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/IR/TypedPointerType.h"

#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint_system.hpp>

#include <nil/blueprint/components/systems/snark/plonk/placeholder/lookup_argument_verifier.hpp>

#include <nil/blueprint/asserts.hpp>
#include <nil/blueprint/stack.hpp>
#include <nil/blueprint/non_native_marshalling.hpp>
#include <nil/blueprint/policy/policy_manager.hpp>
#include <nil/blueprint/extract_constructor_parameters.hpp>

namespace nil {
    namespace blueprint {

        template<typename BlueprintFieldType, typename ArithmetizationParams>
        void handle_lookup_arg_verifier_component(
            const llvm::Instruction *inst,
            stack_frame<crypto3::zk::snark::plonk_variable<typename BlueprintFieldType::value_type>> &frame,
            program_memory<crypto3::zk::snark::plonk_variable<typename BlueprintFieldType::value_type>> &memory,
            circuit<crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType, ArithmetizationParams>> &bp,
            assignment<crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType, ArithmetizationParams>>
                &assignment,
            std::uint32_t start_row) {

            using var = crypto3::zk::snark::plonk_variable<typename BlueprintFieldType::value_type>;

            std::vector<std::size_t> lookup_table_lookup_options_sizes =
                detail::extract_constant_vector<BlueprintFieldType>(inst->getOperand(0));
            std::size_t lookup_table_size = detail::extract_constant_size_t_value<BlueprintFieldType>(inst->getOperand(1));

            std::vector<std::size_t> lookup_table_columns_numbers =
                detail::extract_constant_vector<BlueprintFieldType>(inst->getOperand(2));
            ASSERT(lookup_table_size == detail::extract_constant_size_t_value<BlueprintFieldType>(inst->getOperand(3)));

            std::vector<std::size_t> lookup_gate_constraints_sizes =
                detail::extract_constant_vector<BlueprintFieldType>(inst->getOperand(4));
            std::size_t lookup_gate_size = detail::extract_constant_size_t_value<BlueprintFieldType>(inst->getOperand(5));

            std::vector<std::size_t> lookup_gate_constraints_lookup_input_sizes =
                detail::extract_constant_vector<BlueprintFieldType>(inst->getOperand(6));
            std::size_t lookup_constraints_size = detail::extract_constant_size_t_value<BlueprintFieldType>(inst->getOperand(7)); //sum of the lookup_gate_constraints_sizes elements


            std::vector<std::vector<var>> input_vectors = {};
            std::size_t size = 0;

            for (std::size_t i = 0; i < 9; i++) {
                size = detail::extract_constant_size_t_value<BlueprintFieldType>(inst->getOperand(8 + i * 2 + 1));
                input_vectors.push_back(detail::extract_intrinsic_input_vector<BlueprintFieldType, ArithmetizationParams, var>(
                    inst->getOperand(8 + i * 2), size, frame.scalars, memory, assignment));
            }

            var theta = frame.scalars[inst->getOperand(26)];
            var beta = frame.scalars[inst->getOperand(27)];
            var gamma = frame.scalars[inst->getOperand(28)];
            var L0 = frame.scalars[inst->getOperand(29)];


            std::array<var, 2> V_L_values = {frame.vectors[inst->getOperand(30)][0], frame.vectors[inst->getOperand(30)][1]};
            std::array<var, 2> q_last = {frame.vectors[inst->getOperand(31)][0], frame.vectors[inst->getOperand(31)][1]};
            std::array<var, 2> q_blind = {frame.vectors[inst->getOperand(32)][0], frame.vectors[inst->getOperand(32)][1]};



                using component_type = components::lookup_verifier<
                    crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType, ArithmetizationParams>>;

                const auto p = detail::PolicyManager::get_parameters(detail::ManifestReader<component_type, ArithmetizationParams>::get_witness(
                    0,
                    lookup_gate_size,
                    lookup_gate_constraints_sizes,
                    lookup_gate_constraints_lookup_input_sizes,
                    lookup_table_size,
                    lookup_table_lookup_options_sizes,
                    lookup_table_columns_numbers
                ));

                component_type component_instance =  component_type(p.witness,
                    detail::ManifestReader<component_type, ArithmetizationParams>::get_constants(),
                    detail::ManifestReader<component_type,ArithmetizationParams>::get_public_inputs(),
                    lookup_gate_size,
                    lookup_gate_constraints_sizes,
                    lookup_gate_constraints_lookup_input_sizes,
                    lookup_table_size,
                    lookup_table_lookup_options_sizes,
                    lookup_table_columns_numbers

                    );


                typename component_type::input_type instance_input = {
                    theta,
                    beta,
                    gamma,
                    input_vectors[0],
                    V_L_values,
                    q_last,
                    q_blind,
                    L0,
                    input_vectors[1],
                    input_vectors[2],
                    input_vectors[3],
                    input_vectors[4],
                    input_vectors[5],
                    input_vectors[6],
                    input_vectors[7],
                    input_vectors[8]
                };


                components::generate_circuit(component_instance, bp, assignment, instance_input, start_row);
                std::array<var, 4> res = components::generate_assignments(component_instance, assignment, instance_input, start_row).output;
                frame.vectors[inst] = {res[0], res[1], res[2], res[3]};
        }
    }    // namespace blueprint
}    // namespace nil

#endif    // CRYPTO3_ASSIGNER_RECURSIVE_PROVER_LOOKUP_ARG_VERIFIER_HPP