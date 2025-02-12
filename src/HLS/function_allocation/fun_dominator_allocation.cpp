/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2023 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file fun_dominator_allocation.cpp
 * @brief Function allocation based on the dominator tree of the call graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "fun_dominator_allocation.hpp"

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"

#include "Dominance.hpp"
#include "Parameter.hpp"
#include "application_frontend_flow_step.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "cpu_time.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "library_manager.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "utility.hpp"

#if HAVE_PRAGMA_BUILT && HAVE_EXPERIMENTAL
#include "actor_graph_flow_step.hpp"
#include "actor_graph_flow_step_factory.hpp"
#endif

#include <boost/range/adaptor/reversed.hpp>

fun_dominator_allocation::fun_dominator_allocation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                                   const DesignFlowManagerConstRef _design_flow_manager,
                                                   const HLSFlowStep_Type _hls_flow_step_type)
    : function_allocation(_parameters, _HLSMgr, _design_flow_manager, _hls_flow_step_type), already_executed(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

fun_dominator_allocation::~fun_dominator_allocation() = default;

void fun_dominator_allocation::ComputeRelationships(DesignFlowStepSet& relationship,
                                                    const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));
         const auto frontend_flow_signature = ApplicationFrontendFlowStep::ComputeSignature(BAMBU_FRONTEND_FLOW);
         const auto frontend_flow_step = design_flow_manager.lock()->GetDesignFlowStep(frontend_flow_signature);
         const auto design_flow_step =
             frontend_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(frontend_flow_step)->design_flow_step :
                                  frontend_flow_step_factory->CreateApplicationFrontendFlowStep(BAMBU_FRONTEND_FLOW);
         relationship.insert(design_flow_step);

         const auto technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step ?
                 design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_EXPERIMENTAL && HAVE_PRAGMA_BUILT
         if(parameters->isOption(OPT_parse_pragma) and parameters->getOption<bool>(OPT_parse_pragma) and
            relationship_type == PRECEDENCE_RELATIONSHIP)
         {
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const ActorGraphFlowStepFactory* actor_graph_flow_step_factory =
                GetPointer<const ActorGraphFlowStepFactory>(
                    design_flow_manager.lock()->CGetDesignFlowStepFactory("ActorGraph"));
            const std::string actor_graph_creator_signature =
                ActorGraphFlowStep::ComputeSignature(ACTOR_GRAPHS_CREATOR, input_function, 0, "");
            const vertex actor_graph_creator_step =
                design_flow_manager.lock()->GetDesignFlowStep(actor_graph_creator_signature);
            const DesignFlowStepRef design_flow_step =
                actor_graph_creator_step ?
                    design_flow_graph->CGetDesignFlowStepInfo(actor_graph_creator_step)->design_flow_step :
                    actor_graph_flow_step_factory->CreateActorGraphStep(ACTOR_GRAPHS_CREATOR, input_function);
            relationship.insert(design_flow_step);
         }
#endif
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   function_allocation::ComputeRelationships(relationship, relationship_type);
}

const std::set<std::string> fun_dominator_allocation::simple_functions = {"__builtin_cond_expr32", "llabs",
                                                                          "__builtin_llabs", "labs", "__builtin_labs"};

bool fun_dominator_allocation::HasToBeExecuted() const
{
   return !already_executed;
}

DesignFlowStep_Status fun_dominator_allocation::Exec()
{
   already_executed = true;
   const auto CGM = HLSMgr->GetCallGraphManager();
   auto root_functions = CGM->GetRootFunctions();
   if(parameters->isOption(OPT_top_design_name)) // top design function become the top_vertex
   {
      const auto top_rtldesign_function =
          HLSMgr->get_tree_manager()->GetFunction(parameters->getOption<std::string>(OPT_top_design_name));
      if(top_rtldesign_function && root_functions.count(top_rtldesign_function->index))
      {
         root_functions.clear();
         root_functions.insert(top_rtldesign_function->index);
      }
   }
   if(parameters->isOption(OPT_disable_function_proxy) && parameters->getOption<bool>(OPT_disable_function_proxy))
   {
      return DesignFlowStep_Status::UNCHANGED;
   }

   CustomOrderedSet<unsigned int> reached_from_all;
   for(const auto f_id : root_functions)
   {
      for(const auto reached_f_id : CGM->GetReachedFunctionsFrom(f_id, false))
      {
         reached_from_all.insert(reached_f_id);
      }
   }

   if(!parameters->isOption(OPT_top_functions_names))
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
      for(const auto f_id : root_functions)
      {
         const auto function_behavior = HLSMgr->CGetFunctionBehavior(f_id);
         const auto BH = function_behavior->CGetBehavioralHelper();
         const auto fname = BH->get_function_name();
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---The top function inferred from the specification is: " + fname);
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
   }

   /// the analysis has to be performed only on the reachable functions
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Functions to be synthesized:");
   for(const auto& funID : reached_from_all)
   {
      const auto function_behavior = HLSMgr->CGetFunctionBehavior(funID);
      const auto BH = function_behavior->CGetBehavioralHelper();
      const auto fname = BH->get_function_name();
      if(tree_helper::is_a_nop_function_decl(
             GetPointerS<const function_decl>(HLSMgr->get_tree_manager()->CGetTreeNode(funID))))
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Warning: " + fname + " is empty or the compiler killed all the statements");
      }
      if(BH->has_implementation())
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---" + fname);
      }
      if(!function_behavior->is_simple_pipeline())
      {
         HLSMgr->global_resource_constraints[std::make_pair(fname, WORK_LIBRARY)] = 1;
      }
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }

   for(const auto& top_fid : root_functions)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Allocation thread: " +
                         HLSMgr->CGetFunctionBehavior(top_fid)->CGetBehavioralHelper()->get_function_name());
      const auto top_vertex = CGM->GetVertex(top_fid);
      const auto reached_from_top = CGM->GetReachedFunctionsFrom(top_fid, false);

      CallGraphConstRef subgraph;
      size_t vertex_count;
      boost::tie(vertex_count, subgraph) = [&]() {
         CustomUnorderedSet<vertex> preset;
         preset.insert(top_vertex);
         for(const auto& funID : reached_from_top)
         {
            if(!root_functions.count(funID))
            {
               preset.insert(CGM->GetVertex(funID));
            }
         }
         const auto presub = CGM->CGetCallSubGraph(preset);
         CustomUnorderedSet<vertex> subset;
         subset.insert(top_vertex);
         for(const auto& v : preset)
         {
            if(presub->IsReachable(top_vertex, v))
            {
               subset.insert(v);
            }
         }
         return std::make_pair(subset.size(), CGM->CGetCallSubGraph(subset));
      }();

      if(vertex_count < 2)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Empty thread");
         continue;
      }

      /// we do not need the exit vertex since the post-dominator graph is not used
      const auto cg_dominators =
          refcount<dominance<graph>>(new dominance<graph>(*subgraph, top_vertex, NULL_VERTEX, parameters));
      cg_dominators->calculate_dominance_info(dominance<graph>::CDI_DOMINATORS);
      const auto& cg_dominator_map = cg_dominators->get_dominator_map();
      CustomMap<std::string, CustomOrderedSet<vertex>> fun_dom_map;
      CustomMap<std::string, CustomOrderedSet<unsigned int>> where_used;
      CustomMap<std::string, bool> indirectlyCalled;

      std::list<vertex> topology_sorted_vertex;
      subgraph->TopologicalSort(topology_sorted_vertex);
      for(const auto& current_vertex : topology_sorted_vertex)
      {
         vertex vert_dominator;
         if(boost::in_degree(current_vertex, *subgraph) != 1)
         {
            vert_dominator = cg_dominator_map.at(current_vertex);
         }
         else
         {
            vert_dominator = current_vertex;
         }
         if(boost::out_degree(current_vertex, *subgraph))
         {
            const auto current_id = CGM->get_function(current_vertex);
            THROW_ASSERT(HLSMgr->get_HLS(current_id),
                         "Missing HLS initialization for " +
                             HLSMgr->CGetFunctionBehavior(current_id)->CGetBehavioralHelper()->get_function_name());
            const auto HLS_C = HLSMgr->get_HLS(current_id)->HLS_C;
            THROW_ASSERT(cg_dominator_map.find(current_vertex) != cg_dominator_map.end(),
                         "Dominator vertex not in the dominator tree: " +
                             HLSMgr->CGetFunctionBehavior(current_id)->CGetBehavioralHelper()->get_function_name());
            BOOST_FOREACH(EdgeDescriptor eo, boost::out_edges(current_vertex, *subgraph))
            {
               const auto called_fu_name =
                   functions::GetFUName(CGM->get_function(boost::target(eo, *subgraph)), HLSMgr);
               if(simple_functions.find(called_fu_name) == simple_functions.end() &&
                  (HLS_C->get_number_fu(called_fu_name, WORK_LIBRARY) == INFINITE_UINT || // without constraints
                   HLS_C->get_number_fu(called_fu_name, WORK_LIBRARY) == 1)) // or single instance functions
               {
                  fun_dom_map[called_fu_name].insert(vert_dominator);
                  const auto info = Cget_edge_info<FunctionEdgeInfo, const CallGraph>(eo, *subgraph);

                  if(info->direct_call_points.size())
                  {
                     where_used[called_fu_name].insert(current_id);
                  }
                  else
                  {
                     where_used[called_fu_name];
                  }

                  if(info->indirect_call_points.size() || info->function_addresses.size())
                  {
                     indirectlyCalled[called_fu_name] = true;
                  }
                  else
                  {
                     indirectlyCalled[called_fu_name] = false;
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "---Dominator vertex: " +
                                  HLSMgr->CGetFunctionBehavior(CGM->get_function(vert_dominator))
                                      ->CGetBehavioralHelper()
                                      ->get_function_name() +
                                  " => " + called_fu_name);
            }
         }
      }
      /// compute the number of instances for each function
      CustomMap<std::string, unsigned int> num_instances;
      num_instances[functions::GetFUName(top_fid, HLSMgr)] = 1;
      CustomMap<unsigned int, std::vector<std::string>> function_allocation_map;
      for(const auto& cur : topology_sorted_vertex)
      {
         const auto cur_id = CGM->get_function(cur);
         const auto cur_fu_name = functions::GetFUName(cur_id, HLSMgr);
         if(reached_from_top.find(cur_id) == reached_from_top.end())
         {
            continue;
         }
         THROW_ASSERT(num_instances.count(cur_fu_name),
                      "missing number of instances of function " +
                          HLSMgr->CGetFunctionBehavior(cur_id)->CGetBehavioralHelper()->get_function_name());
         function_allocation_map[cur_id];
         const auto cur_instances = num_instances.at(cur_fu_name);
         BOOST_FOREACH(EdgeDescriptor eo, boost::out_edges(cur, *subgraph))
         {
            const auto tgt = boost::target(eo, *subgraph);
            const auto tgt_fu_name = functions::GetFUName(CGM->get_function(tgt), HLSMgr);
            const auto n_call_points = static_cast<unsigned int>(
                Cget_edge_info<FunctionEdgeInfo, const CallGraph>(eo, *subgraph)->direct_call_points.size());
            if(num_instances.find(tgt_fu_name) == num_instances.end())
            {
               num_instances[tgt_fu_name] = cur_instances * n_call_points;
            }
            else
            {
               num_instances[tgt_fu_name] += cur_instances * n_call_points;
            }
         }
      }

      THROW_ASSERT(num_instances.at(functions::GetFUName(top_fid, HLSMgr)) == 1,
                   "top function cannot be called from some other function");
      /// find the common dominator and decide where to allocate
      for(const auto& dom_map : fun_dom_map)
      {
         unsigned int cur_id = 0;
         if(dom_map.second.size() == 1)
         {
            auto cur = *dom_map.second.begin();
            while(num_instances.at(functions::GetFUName(CGM->get_function(cur), HLSMgr)) != 1)
            {
               cur = cg_dominator_map.at(cur);
            }
            cur_id = CGM->get_function(cur);
         }
         else
         {
            if(dom_map.second.count(top_vertex))
            {
               cur_id = top_fid;
            }
            else
            {
               auto vert_it = dom_map.second.begin();
               const auto vert_it_end = dom_map.second.end();
               std::list<vertex> dominator_list1;
               vertex cur = *vert_it;
               dominator_list1.push_front(cur);
               do
               {
                  THROW_ASSERT(cg_dominator_map.count(cur), "");
                  cur = cg_dominator_map.at(cur);
                  dominator_list1.push_front(cur);
               } while(cur != top_vertex);
               ++vert_it;
               auto last = dominator_list1.end();
               for(; vert_it != vert_it_end; ++vert_it)
               {
                  std::list<vertex> dominator_list2;
                  cur = *vert_it;
                  dominator_list2.push_front(cur);
                  do
                  {
                     THROW_ASSERT(cg_dominator_map.count(cur), "");
                     cur = cg_dominator_map.at(cur);
                     dominator_list2.push_front(cur);
                  } while(cur != top_vertex);
                  /// find the common dominator between two candidates
                  auto dl1_it = dominator_list1.begin(), dl2_it = dominator_list2.begin(),
                       dl2_it_end = dominator_list2.end(), cur_last = dominator_list1.begin();
                  while(dl1_it != last && dl2_it != dl2_it_end && *dl1_it == *dl2_it &&
                        (num_instances.at(functions::GetFUName(CGM->get_function(*dl1_it), HLSMgr)) == 1))
                  {
                     cur = *dl1_it;
                     ++dl1_it;
                     cur_last = dl1_it;
                     ++dl2_it;
                  }
                  last = cur_last;
                  cur_id = CGM->get_function(cur);
                  if(cur == top_vertex)
                  {
                     break;
                  }
               }
            }
         }
         THROW_ASSERT(cur_id, "null function id index unexpected");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---Function " + functions::GetFUName(dom_map.first, HLSMgr) + " may be allocated in " +
                            HLSMgr->CGetFunctionBehavior(cur_id)->CGetBehavioralHelper()->get_function_name());
         function_allocation_map[cur_id].push_back(dom_map.first);
      }

      /// really allocate
      const auto HLS_T = HLSMgr->get_HLS_target();
      const auto TechM = HLS_T->get_technology_manager();

      for(const auto& funID : reached_from_top)
      {
         for(const auto& fu_name : function_allocation_map.at(funID))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "Allocating " + fu_name + " in " +
                               HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name());
            THROW_ASSERT(where_used.at(fu_name).size() > 0 || indirectlyCalled.at(fu_name),
                         fu_name + " not used anywhere");
            if(where_used.at(fu_name).size() == 1)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "---Skipped because called only by one other function");
               continue;
            }
            const auto fnode = HLSMgr->get_tree_manager()->GetFunction(fu_name);
            if(fnode && HLSMgr->hasToBeInterfaced(fnode->index))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Skipped because it has to be interfaced");
               continue;
            }
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT
            if(fun_name == "panda_pthread_mutex")
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Skipped because is a pthread mutex call");
               continue;
            }
#endif
            const auto library_name = TechM->get_library(fu_name);
            if(library_name != "")
            {
               const auto libraryManager = TechM->get_library_manager(library_name);
               const auto techNode_obj = libraryManager->get_fu(fu_name);
               if(GetPointer<functional_unit_template>(techNode_obj))
               {
                  continue;
               }
            }

            HLSMgr->Rfuns->map_shared_function(funID, fu_name);

            const auto cur_function_behavior = HLSMgr->CGetFunctionBehavior(funID);
            const auto cur_BH = cur_function_behavior->CGetBehavioralHelper();
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                           "---Adding proxy wrapper in function " + cur_BH->get_function_name());
            /// add proxies
            for(const auto& wu_id : where_used.at(fu_name))
            {
               if(wu_id != funID)
               {
                  const auto wiu_function_behavior = HLSMgr->CGetFunctionBehavior(wu_id);
                  const auto wiu_BH = wiu_function_behavior->CGetBehavioralHelper();
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                 "---Adding proxy function in function " + wiu_BH->get_function_name());
                  HLSMgr->Rfuns->add_shared_function_proxy(wu_id, fu_name);
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   }

   return DesignFlowStep_Status::SUCCESS;
}
