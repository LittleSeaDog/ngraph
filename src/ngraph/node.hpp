// ----------------------------------------------------------------------------
// Copyright 2017 Nervana Systems Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// ----------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <iostream>

#include "ngraph/autodiff/adjoints.hpp"
#include "ngraph/common.hpp"
#include "ngraph/descriptor/input.hpp"
#include "ngraph/descriptor/output.hpp"
#include "ngraph/descriptor/tensor.hpp"
#include "ngraph/types/type.hpp"

namespace ngraph
{
    /// Nodes are the backbone of the graph of Value dataflow. Every node has
    /// zero or more nodes as arguments and one value, which is either a tensor
    /// view or a (possibly empty) tuple of values.
    class Node : public std::enable_shared_from_this<Node>
    {
        // So Adjoints can call generate_adjoints
        friend class autodiff::Adjoints;

    protected:
        Node(const Nodes& arguments, std::shared_ptr<const ValueType> value_type = nullptr);
        Node();
        Node(std::shared_ptr<const ValueType> value_type);

        virtual ~Node();

        virtual void generate_adjoints(autodiff::Adjoints& adjoints,
                                       const std::shared_ptr<Node>& delta)
        {
        }

    public:
        /// The class name, must not contain spaces
        virtual std::string description() const = 0;
        std::string get_name() const;
        void set_name(const std::string& name);

        /// Propagate types and check arguments for consistency
        virtual void propagate_types() = 0;

        /// Assign Input and Output vectors
        // This might later need to be virtual.
        void assign_tensors();

        const Nodes& get_arguments() const { return m_arguments; }
        void clear_arguments() { m_arguments.clear(); }
        const std::multiset<Node*>& users() const { return m_users; }
        virtual std::string get_node_id() const;

        /// Return true if this has the same implementing class as node. This
        /// will be used by the pattern matcher when comparing a pattern
        /// graph against the graph.
        bool is_same_op_type(const std::shared_ptr<Node>& node) const
        {
            Node* n = node.get();
            return std::type_index(typeid(*this)) == std::type_index(typeid(*n));
        }

        std::shared_ptr<const ValueType> get_value_type();
        const std::shared_ptr<const ValueType> get_value_type() const;
        void set_value_type(const element::Type& element_type, const Shape& shape)
        {
            m_value_type = std::make_shared<TensorViewType>(element_type, shape);
        }

        void set_value_type(const std::shared_ptr<const ValueType>& value_type)
        {
            m_value_type = value_type;
        }

        // Set the value type if it has not already been set; otherwise, ensure that
        // value_type agrees with the value type that was set.
        // This is used when the framework specifies a value type for the value, and we
        // independently compute what we thing the value type should be from the arguments.
        void set_value_type_checked(const std::shared_ptr<const ValueType>& value_type);

        bool is_parameter() const;
        bool is_output() const;
        void set_is_output();

        size_t get_instance_id() const { return m_instance_id; }
        friend std::ostream& operator<<(std::ostream&, const Node&);

        std::deque<descriptor::Input>& get_inputs() { return m_inputs; }
        const std::deque<descriptor::Input>& get_inputs() const { return m_inputs; }
        std::deque<descriptor::Output>& get_outputs() { return m_outputs; }
        const std::deque<descriptor::Output>& get_outputs() const { return m_outputs; }
        std::unordered_set<descriptor::Tensor*> liveness_live_list;
        std::unordered_set<descriptor::Tensor*> liveness_new_list;
        std::unordered_set<descriptor::Tensor*> liveness_free_list;

        std::shared_ptr<Node> backprop_node(const std::shared_ptr<Node>& x,
                                            const std::shared_ptr<Node>& c);

        /// Returns the shape if this node has tensor type, othetwise error.
        const Shape& get_shape() const { return m_value_type->get_shape(); }
    protected:
        Nodes m_arguments;
        std::shared_ptr<const ValueType> m_value_type;
        std::multiset<Node*> m_users;
        std::string m_name;
        size_t m_instance_id;
        static std::atomic<size_t> m_next_instance_id;
        std::deque<descriptor::Input> m_inputs;
        std::deque<descriptor::Output> m_outputs;
        bool m_is_output;
        std::unordered_map<Node*, autodiff::Adjoints> m_adjoint_map;
    };
}
