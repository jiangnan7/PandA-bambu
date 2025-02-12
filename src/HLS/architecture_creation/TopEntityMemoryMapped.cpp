/*
 *                 _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *               _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *              _/      _/    _/ _/    _/ _/   _/ _/    _/
 *             _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *           ***********************************************
 *                            PandA Project
 *                   URL: http://panda.dei.polimi.it
 *                     Politecnico di Milano - DEIB
 *                      System Architectures Group
 *           ***********************************************
 *            Copyright (C) 2004-2023 Politecnico di Milano
 *
 * This file is part of the PandA framework.
 *
 * The PandA framework is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// Header include
#include "TopEntityMemoryMapped.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "call_graph_manager.hpp"

/// circuit includes
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS/binding/module include
#include "fu_binding.hpp"

/// HLS/function_allocation include
#include "functions.hpp"

/// HLS/memory includes
#include "memory.hpp"
#include "memory_allocation.hpp"
#include "memory_cs.hpp"
#include "memory_symbol.hpp"

/// STL include
#include "custom_map.hpp"

/// technology include
#include "technology_manager.hpp"

/// technology/physical_library include
#include "technology_node.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"

#include "copyrights_strings.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

static void propagateInterface(structural_managerRef SM, structural_objectRef wrappedObj,
                               std::list<std::string>& ParametersName);

TopEntityMemoryMapped::TopEntityMemoryMapped(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                             unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : top_entity(_parameters, _HLSMgr, _funId, _design_flow_manager,
                 HLSFlowStep_Type::TOP_ENTITY_MEMORY_MAPPED_CREATION),
      ParametersName()
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

TopEntityMemoryMapped::~TopEntityMemoryMapped() = default;

void TopEntityMemoryMapped::Initialize()
{
   top_entity::Initialize();
   const auto CGM = HLSMgr->CGetCallGraphManager();
   const auto& top_function_ids = CGM->GetRootFunctions();
   is_root_function = top_function_ids.count(funId);
   const auto is_wb4_root = is_root_function && parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) ==
                                                    HLSFlowStep_Type::WB4_INTERFACE_GENERATION;
   const auto is_addressed_fun = HLSMgr->hasToBeInterfaced(funId) && !is_root_function;
   needMemoryMappedRegisters = is_wb4_root || is_addressed_fun || parameters->getOption<bool>(OPT_memory_mapped_top);
   AddedComponents.clear();
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   _channels_number = FB->GetChannelsNumber();
   _channels_type = FB->GetChannelsType();
}

DesignFlowStep_Status TopEntityMemoryMapped::InternalExec()
{
   top_entity::InternalExec();
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);

   const auto& function_parameters = FB->CGetBehavioralHelper()->get_parameters();
   for(const auto& function_parameter : function_parameters)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing parameter " + STR(function_parameter));
      ParametersName.push_back(FB->CGetBehavioralHelper()->PrintVariable(function_parameter));
   }

   if(needMemoryMappedRegisters)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Allocating parameters ");
      allocate_parameters();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Allocated parameters ");
   }

   const auto wrappedObj = HLS->top->get_circ();
   const auto module_name = wrappedObj->get_id();
   wrappedObj->set_id(module_name + "_int");

   const structural_managerRef SM_mm_interface(new structural_manager(parameters));

   const structural_type_descriptorRef module_type(new structural_type_descriptor(module_name + "_int"));
   SM_mm_interface->set_top_info(module_name, wrappedObj->get_typeRef());
   wrappedObj->set_type(module_type);
   HLS->top->set_top_info(module_name + "_int", module_type);
   const auto interfaceObj = SM_mm_interface->get_circ();

   // add the core to the wrapper
   wrappedObj->set_owner(interfaceObj);
   wrappedObj->set_id(wrappedObj->get_id() + "_i0");

   GetPointerS<module>(interfaceObj)->add_internal_object(wrappedObj);
   // Set some descriptions and legal stuff
   GetPointerS<module>(interfaceObj)
       ->set_description("Memory mapped interface for top component: " + wrappedObj->get_typeRef()->id_type);
   GetPointerS<module>(interfaceObj)->set_copyright(GENERATED_COPYRIGHT);
   GetPointerS<module>(interfaceObj)->set_authors("Component automatically generated by bambu");
   GetPointerS<module>(interfaceObj)->set_license(GENERATED_LICENSE);

   propagateInterface(SM_mm_interface, wrappedObj, ParametersName);

   HLS->Rfu->manage_module_ports(HLSMgr, HLS, SM_mm_interface, wrappedObj, 0);

   AddedComponents.push_back(wrappedObj);
   if(needMemoryMappedRegisters)
   {
      insertStatusRegister(SM_mm_interface, wrappedObj);

      insertMemoryMappedRegister(SM_mm_interface, wrappedObj);

      insertStartDoneLogic(SM_mm_interface, wrappedObj);
   }
   else
   {
      forwardPorts(SM_mm_interface, wrappedObj);
   }

   unsigned int unique_id = 0;
   HLS->Rfu->manage_memory_ports_parallel_chained(HLSMgr, SM_mm_interface, AddedComponents, interfaceObj, HLS,
                                                  unique_id);

   memory::propagate_memory_parameters(interfaceObj, SM_mm_interface);

   HLS->top = SM_mm_interface;
   return DesignFlowStep_Status::SUCCESS;
}

void TopEntityMemoryMapped::resizing_IO(module* fu_module, unsigned int max_n_ports) const
{
   const auto bus_addr_bitsize = HLSMgr->get_address_bitsize();
   const auto bus_data_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
   const auto bus_size_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
   const auto bus_tag_bitsize =
       HLS->Param->isOption(OPT_context_switch) ? GetPointerS<memory_cs>(HLSMgr->Rmem)->get_bus_tag_bitsize() : 0U;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Resizing input ports");
   for(auto i = 0U; i < fu_module->get_in_port_size(); i++)
   {
      const auto port = fu_module->get_in_port(i);
      if(port->get_kind() == port_vector_o_K && GetPointerS<port_o>(port)->get_ports_size() == 0)
      {
         GetPointerS<port_o>(port)->add_n_ports(max_n_ports, port);
      }

      if(GetPointerS<port_o>(port)->get_is_data_bus() || GetPointerS<port_o>(port)->get_is_addr_bus() ||
         GetPointerS<port_o>(port)->get_is_size_bus() || GetPointerS<port_o>(port)->get_is_tag_bus())
      {
         port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Resized input ports");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Resizing output ports");
   for(auto i = 0U; i < fu_module->get_out_port_size(); i++)
   {
      const auto port = fu_module->get_out_port(i);
      if(port->get_kind() == port_vector_o_K && GetPointerS<port_o>(port)->get_ports_size() == 0)
      {
         GetPointerS<port_o>(port)->add_n_ports(max_n_ports, port);
      }
      if(GetPointerS<port_o>(port)->get_is_data_bus() || GetPointerS<port_o>(port)->get_is_addr_bus() ||
         GetPointerS<port_o>(port)->get_is_size_bus() || GetPointerS<port_o>(port)->get_is_tag_bus())
      {
         port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Resized output ports");
}

void TopEntityMemoryMapped::allocate_parameters() const
{
   const auto function_behavior = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();

   // Allocate memory for the start register.
   auto base_address = HLSMgr->Rmem->get_symbol(funId, funId)->get_symbol_name();
   memory::add_memory_parameter(HLS->top, base_address, STR(HLSMgr->Rmem->get_parameter_base_address(funId, funId)));

   // Allocate every parameter on chip.
   for(const auto& topParam : behavioral_helper->get_parameters())
   {
      base_address = HLSMgr->Rmem->get_symbol(topParam, funId)->get_symbol_name();
      memory::add_memory_parameter(HLS->top, base_address,
                                   STR(HLSMgr->Rmem->get_parameter_base_address(funId, topParam)));
   }

   // Allocate the return value on chip.
   const unsigned int function_return = behavioral_helper->GetFunctionReturnType(HLS->functionId);
   if(function_return)
   {
      base_address = HLSMgr->Rmem->get_symbol(function_return, funId)->get_symbol_name();
      memory::add_memory_parameter(HLS->top, base_address,
                                   STR(HLSMgr->Rmem->get_parameter_base_address(funId, function_return)));
   }
}

static void connect_with_signal_name(structural_managerRef SM, structural_objectRef portA, structural_objectRef portB,
                                     std::string signalName);

static void connectClockAndReset(structural_managerRef SM, structural_objectRef interfaceObj,
                                 structural_objectRef component);

static void setBusSizes(structural_objectRef component, const HLS_managerRef HLSMgr);
void TopEntityMemoryMapped::insertMemoryMappedRegister(structural_managerRef SM_mm, structural_objectRef wrappedObj)
{
   const auto interfaceObj = SM_mm->get_circ();

   const auto function_parameters = HLSMgr->Rmem->get_function_parameters(HLS->functionId);
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();

   const auto controlSignal = interfaceObj->find_member("ControlSignal", signal_o_K, interfaceObj);
   const auto multi_channel_bus = _channels_type == MemoryAllocation_ChannelsType::MEM_ACC_NN;

   for(const auto& function_parameter : function_parameters)
   {
      // Do not consider return register and status register here.
      if(function_parameter.first == BH->GetFunctionReturnType(HLS->functionId) ||
         function_parameter.first == HLS->functionId)
      {
         continue;
      }

      const auto signalName = BH->PrintVariable(function_parameter.first);
      const auto component_name = multi_channel_bus ? MEMORY_MAPPED_REGISTERN_FU : MEMORY_MAPPED_REGISTER_FU;
      const auto memoryMappedRegister =
          SM_mm->add_module_from_technology_library("mm_register_" + signalName, component_name,
                                                    HLS->HLS_T->get_technology_manager()->get_library(component_name),
                                                    interfaceObj, HLS->HLS_T->get_technology_manager());
      if(multi_channel_bus)
      {
         resizing_IO(GetPointerS<module>(memoryMappedRegister), _channels_number);
      }
      GetPointerS<module>(memoryMappedRegister)
          ->SetParameter("ALLOCATED_ADDRESS",
                         HLSMgr->Rmem->get_symbol(function_parameter.first, HLS->functionId)->get_symbol_name());
      setBusSizes(memoryMappedRegister, HLSMgr);

      connectClockAndReset(SM_mm, interfaceObj, memoryMappedRegister);

      HLS->Rfu->manage_module_ports(HLSMgr, HLS, SM_mm, memoryMappedRegister, 0);

      AddedComponents.push_back(memoryMappedRegister);
      const auto parameterPort = wrappedObj->find_member(signalName, port_o_K, wrappedObj);
      // out1
      const auto port_out1 = memoryMappedRegister->find_member("out1", port_o_K, memoryMappedRegister);
      GetPointerS<port_o>(port_out1)->type_resize(STD_GET_SIZE(parameterPort->get_typeRef()));

      if(!is_root_function)
      {
         // insert wDataMux
         const auto pMux = SM_mm->add_module_from_technology_library(
             "pMux_" + signalName, MUX_GATE_STD, HLS->HLS_T->get_technology_manager()->get_library(MUX_GATE_STD),
             interfaceObj, HLS->HLS_T->get_technology_manager());

         const auto pMuxIn1 = pMux->find_member("in1", port_o_K, pMux);
         GetPointerS<port_o>(pMuxIn1)->type_resize(STD_GET_SIZE(port_out1->get_typeRef()));
         connect_with_signal_name(SM_mm, port_out1, pMuxIn1, "registerToMux" + signalName);

         const auto pMuxIn2 = pMux->find_member("in2", port_o_K, pMux);
         GetPointerS<port_o>(pMuxIn2)->type_resize(
             STD_GET_SIZE(interfaceObj->find_member(signalName, port_o_K, interfaceObj)->get_typeRef()));
         SM_mm->add_connection(interfaceObj->find_member(signalName, port_o_K, interfaceObj), pMuxIn2);

         const auto pMuxOut1 = pMux->find_member("out1", port_o_K, pMux);
         GetPointerS<port_o>(pMuxOut1)->type_resize(STD_GET_SIZE(parameterPort->get_typeRef()));
         const auto sign = SM_mm->add_sign("muxTo" + signalName, SM_mm->get_circ(), pMuxOut1->get_typeRef());

         SM_mm->add_connection(pMuxOut1, sign);
         SM_mm->add_connection(sign, parameterPort);

         SM_mm->add_connection(pMux->find_member("sel", port_o_K, pMux), controlSignal);
      }
      else
      {
         connect_with_signal_name(SM_mm, port_out1, parameterPort, "ConnectTo" + signalName);
      }
   }

   if(!BH->GetFunctionReturnType(HLS->functionId))
   {
      return;
   }

   const auto returnType = BH->GetFunctionReturnType(HLS->functionId);
   const auto component_name = multi_channel_bus ? RETURN_MM_REGISTERN_FU : RETURN_MM_REGISTER_FU;
   const auto returnRegister =
       SM_mm->add_module_from_technology_library("mm_register_" + STR(RETURN_PORT_NAME), component_name,
                                                 HLS->HLS_T->get_technology_manager()->get_library(component_name),
                                                 interfaceObj, HLS->HLS_T->get_technology_manager());
   if(multi_channel_bus)
   {
      resizing_IO(GetPointer<module>(returnRegister), _channels_number);
   }
   GetPointer<module>(returnRegister)
       ->SetParameter("ALLOCATED_ADDRESS", HLSMgr->Rmem->get_symbol(returnType, HLS->functionId)->get_symbol_name());
   setBusSizes(returnRegister, HLSMgr);
   connectClockAndReset(SM_mm, interfaceObj, returnRegister);

   AddedComponents.push_back(returnRegister);

   const auto innerReturnPort = wrappedObj->find_member(RETURN_PORT_NAME, port_o_K, wrappedObj);

   const auto outerReturnPort = interfaceObj->find_member(RETURN_PORT_NAME, port_o_K, interfaceObj);

   const auto returnInPort = returnRegister->find_member("in1", port_o_K, returnRegister);
   GetPointer<port_o>(returnInPort)->set_type(innerReturnPort->get_typeRef());

   const auto returnOutPort = returnRegister->find_member("out1", port_o_K, returnRegister);
   GetPointer<port_o>(returnOutPort)->set_type(outerReturnPort->get_typeRef());

   connect_with_signal_name(SM_mm, returnInPort, innerReturnPort, "retToRegister");
   connect_with_signal_name(SM_mm, returnOutPort, outerReturnPort, "registerToRet");

   const auto donePortSignal = interfaceObj->find_member("DonePortSignal", signal_o_K, interfaceObj);
   SM_mm->add_connection(donePortSignal, returnRegister->find_member(DONE_PORT_NAME, port_o_K, returnRegister));

   HLS->Rfu->manage_module_ports(HLSMgr, HLS, SM_mm, returnRegister, 0);
}

void TopEntityMemoryMapped::insertStartDoneLogic(structural_managerRef SM_mm, structural_objectRef wrappedObj)
{
   const auto interfaceObj = SM_mm->get_circ();

   if(!is_root_function)
   {
      const auto startOr = SM_mm->add_module_from_technology_library(
          "startOr", OR_GATE_STD, HLS->HLS_T->get_technology_manager()->get_library(OR_GATE_STD), interfaceObj,
          HLS->HLS_T->get_technology_manager());
      const auto port_startOr = startOr->find_member("in", port_o_K, startOr);
      auto* inPortStartOr = GetPointer<port_o>(port_startOr);
      inPortStartOr->add_n_ports(2, port_startOr);

      SM_mm->add_connection(interfaceObj->find_member(START_PORT_NAME, port_o_K, interfaceObj),
                            inPortStartOr->get_port(0));

      // output
      connect_with_signal_name(SM_mm, startOr->find_member("out1", port_o_K, startOr),
                               wrappedObj->find_member(START_PORT_NAME, port_o_K, wrappedObj), "StartOrOut");

      const auto statusRegisterStart = interfaceObj->find_member("statusRegisterStart", signal_o_K, interfaceObj);
      SM_mm->add_connection(statusRegisterStart, inPortStartOr->get_port(1));
   }
   else
   {
      SM_mm->add_connection(interfaceObj->find_member("statusRegisterStart", signal_o_K, interfaceObj),
                            wrappedObj->find_member(START_PORT_NAME, port_o_K, wrappedObj));
   }

   const auto donePortSignal = interfaceObj->find_member("DonePortSignal", signal_o_K, interfaceObj);
   SM_mm->add_connection(donePortSignal, interfaceObj->find_member(DONE_PORT_NAME, port_o_K, interfaceObj));

   if(HLSMgr->CGetCallGraphManager()->ExistsAddressedFunction())
   {
      const auto multi_channel_bus = _channels_type == MemoryAllocation_ChannelsType::MEM_ACC_NN;
      const auto component_name = multi_channel_bus ? NOTYFY_CALLER_MINIMALN_FU : NOTYFY_CALLER_MINIMAL_FU;
      const auto notifyCaller = SM_mm->add_module_from_technology_library(
          "notifyCaller", component_name, HLS->HLS_T->get_technology_manager()->get_library(component_name),
          interfaceObj, HLS->HLS_T->get_technology_manager());
      if(multi_channel_bus)
      {
         resizing_IO(GetPointerS<module>(notifyCaller), _channels_number);
      }
      setBusSizes(notifyCaller, HLSMgr);
      connectClockAndReset(SM_mm, interfaceObj, notifyCaller);
      HLS->Rfu->manage_module_ports(HLSMgr, HLS, SM_mm, notifyCaller, 0);
      AddedComponents.push_back(notifyCaller);

      const auto NotifiedSignal = interfaceObj->find_member("Notified", signal_o_K, interfaceObj);
      SM_mm->add_connection(notifyCaller->find_member("notified", port_o_K, notifyCaller), NotifiedSignal);

      // Connect notify address signal and donePortSignal
      SM_mm->add_connection(donePortSignal, notifyCaller->find_member(DONE_PORT_NAME, port_o_K, notifyCaller));
      const auto notifyAddressSignal = interfaceObj->find_member("NotifyAddressSignal", signal_o_K, interfaceObj);
      SM_mm->add_connection(notifyAddressSignal, notifyCaller->find_member("notifyAddress", port_o_K, notifyCaller));
   }
}

void TopEntityMemoryMapped::insertStatusRegister(structural_managerRef SM_mm, structural_objectRef wrappedObj)
{
   const auto multi_channel_bus = _channels_type == MemoryAllocation_ChannelsType::MEM_ACC_NN;
   const auto interfaceObj = SM_mm->get_circ();
   if(HLSMgr->CGetCallGraphManager()->ExistsAddressedFunction())
   {
      const auto component_name = multi_channel_bus ? STATUS_REGISTERN_FU : STATUS_REGISTER_FU;
      const auto statusRegister = SM_mm->add_module_from_technology_library(
          "StatusRegister", component_name, HLS->HLS_T->get_technology_manager()->get_library(component_name),
          interfaceObj, HLS->HLS_T->get_technology_manager());
      if(multi_channel_bus)
      {
         resizing_IO(GetPointer<module>(statusRegister), _channels_number);
      }
      GetPointerS<module>(statusRegister)
          ->SetParameter("ALLOCATED_ADDRESS",
                         HLSMgr->Rmem->get_symbol(HLS->functionId, HLS->functionId)->get_symbol_name());
      setBusSizes(statusRegister, HLSMgr);
      connectClockAndReset(SM_mm, interfaceObj, statusRegister);

      HLS->Rfu->manage_module_ports(HLSMgr, HLS, SM_mm, statusRegister, 0);
      AddedComponents.push_back(statusRegister);

      const auto controlPort = statusRegister->find_member("control", port_o_K, statusRegister);
      const auto ControlSignal = SM_mm->add_sign("ControlSignal", interfaceObj, controlPort->get_typeRef());
      SM_mm->add_connection(ControlSignal, controlPort);

      const auto donePort = wrappedObj->find_member(DONE_PORT_NAME, port_o_K, wrappedObj);
      const auto donePortSignal = SM_mm->add_sign("DonePortSignal", interfaceObj, donePort->get_typeRef());
      SM_mm->add_connection(donePort, donePortSignal);
      SM_mm->add_connection(statusRegister->find_member(DONE_PORT_NAME, port_o_K, statusRegister), donePortSignal);

      const auto notifyAddressPort = statusRegister->find_member("notifyAddress", port_o_K, statusRegister);
      const auto notifyAddressSignal =
          SM_mm->add_sign("NotifyAddressSignal", interfaceObj, notifyAddressPort->get_typeRef());
      notifyAddressSignal->set_type(notifyAddressPort->get_typeRef());
      SM_mm->add_connection(notifyAddressPort, notifyAddressSignal);

      const auto notifiedPort = statusRegister->find_member("notified", port_o_K, statusRegister);
      const auto NotifiedSignal = SM_mm->add_sign("Notified", interfaceObj, notifiedPort->get_typeRef());
      NotifiedSignal->set_type(notifiedPort->get_typeRef());
      SM_mm->add_connection(notifiedPort, NotifiedSignal);

      const auto startPort = statusRegister->find_member(START_PORT_NAME, port_o_K, statusRegister);
      const auto statusRegisterStart = SM_mm->add_sign("statusRegisterStart", interfaceObj, startPort->get_typeRef());
      SM_mm->add_connection(statusRegisterStart, startPort);
   }
   else
   {
      const auto component_name = multi_channel_bus ? STATUS_REGISTER_NO_NOTIFIEDN_FU : STATUS_REGISTER_NO_NOTIFIED_FU;
      const auto statusRegister = SM_mm->add_module_from_technology_library(
          "StatusRegister", component_name, HLS->HLS_T->get_technology_manager()->get_library(component_name),
          interfaceObj, HLS->HLS_T->get_technology_manager());
      if(multi_channel_bus)
      {
         resizing_IO(GetPointer<module>(statusRegister), _channels_number);
      }
      GetPointerS<module>(statusRegister)
          ->SetParameter("ALLOCATED_ADDRESS",
                         HLSMgr->Rmem->get_symbol(HLS->functionId, HLS->functionId)->get_symbol_name());
      setBusSizes(statusRegister, HLSMgr);
      connectClockAndReset(SM_mm, interfaceObj, statusRegister);

      HLS->Rfu->manage_module_ports(HLSMgr, HLS, SM_mm, statusRegister, 0);
      AddedComponents.push_back(statusRegister);

      const auto donePort = wrappedObj->find_member(DONE_PORT_NAME, port_o_K, wrappedObj);
      const auto donePortSignal = SM_mm->add_sign("DonePortSignal", interfaceObj, donePort->get_typeRef());
      SM_mm->add_connection(donePort, donePortSignal);
      SM_mm->add_connection(statusRegister->find_member(DONE_PORT_NAME, port_o_K, statusRegister), donePortSignal);

      const auto startPort = statusRegister->find_member(START_PORT_NAME, port_o_K, statusRegister);
      const auto statusRegisterStart = SM_mm->add_sign("statusRegisterStart", interfaceObj, startPort->get_typeRef());
      SM_mm->add_connection(statusRegisterStart, startPort);
   }
}

void TopEntityMemoryMapped::forwardPorts(structural_managerRef SM_mm, structural_objectRef wrappedObj)
{
   const auto interfaceObj = SM_mm->get_circ();

   // Start and done
   SM_mm->add_connection(interfaceObj->find_member(START_PORT_NAME, port_o_K, interfaceObj),
                         wrappedObj->find_member(START_PORT_NAME, port_o_K, wrappedObj));
   SM_mm->add_connection(interfaceObj->find_member(DONE_PORT_NAME, port_o_K, interfaceObj),
                         wrappedObj->find_member(DONE_PORT_NAME, port_o_K, wrappedObj));

   for(const auto& Itr : ParametersName)
   {
      const auto outerPort = interfaceObj->find_member(Itr, port_o_K, interfaceObj);
      const auto innerPort = wrappedObj->find_member(Itr, port_o_K, wrappedObj);
      SM_mm->add_connection(innerPort, outerPort);
   }

   // If the function does not have a return port end.
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   if(!BH->GetFunctionReturnType(HLS->functionId))
   {
      return;
   }

   SM_mm->add_connection(interfaceObj->find_member(RETURN_PORT_NAME, port_o_K, interfaceObj),
                         wrappedObj->find_member(RETURN_PORT_NAME, port_o_K, wrappedObj));
}

static void propagateInterface(structural_managerRef SM, structural_objectRef wrappedObj,
                               std::list<std::string>& ParametersName)
{
   THROW_ASSERT(wrappedObj, "Null wrapped object");
   const auto interfaceObj = SM->get_circ();

   const auto wrappedModule = GetPointer<module>(wrappedObj);
   for(unsigned int i = 0; i < wrappedModule->get_num_ports(); ++i)
   {
      const auto port = wrappedModule->get_positional_port(i);
      const auto portObj = GetPointer<port_o>(port);

      const auto portID = portObj->get_id();
      if(portID != CLOCK_PORT_NAME && portID != RESET_PORT_NAME && portID != START_PORT_NAME &&
         portID != RETURN_PORT_NAME && portID != DONE_PORT_NAME &&
         std::find(ParametersName.begin(), ParametersName.end(), portID) == ParametersName.end())
      {
         continue;
      }

      SM->add_port(portID, portObj->get_port_direction(), interfaceObj, portObj->get_typeRef());
   }

   connectClockAndReset(SM, interfaceObj, wrappedObj);
}

static void connect_with_signal_name(structural_managerRef SM, structural_objectRef portA, structural_objectRef portB,
                                     std::string signalName)
{
   const auto signA = GetPointerS<port_o>(portA)->get_connected_signal();
   const auto signB = GetPointerS<port_o>(portB)->get_connected_signal();

   if(!signA && !signB)
   {
      structural_objectRef sign;
      if(GetPointerS<port_o>(portA)->get_port_size() > GetPointerS<port_o>(portB)->get_port_size())
      {
         sign = SM->add_sign(signalName, SM->get_circ(), portA->get_typeRef());
      }
      else
      {
         sign = SM->add_sign(signalName, SM->get_circ(), portB->get_typeRef());
      }

      SM->add_connection(portA, sign);
      SM->add_connection(sign, portB);
   }
   else if(!signA && signB)
   {
      SM->add_connection(portA, signB);
   }
   else if(!signB && signA)
   {
      SM->add_connection(portB, signA);
   }
   else if(signA && signB)
   {
      SM->add_connection(signA, signB);
   }
}

static void connectClockAndReset(structural_managerRef SM, structural_objectRef interfaceObj,
                                 structural_objectRef component)
{
   // Clock and Reset connection
   const auto port_ck = component->find_member(CLOCK_PORT_NAME, port_o_K, component);
   const auto clock = interfaceObj->find_member(CLOCK_PORT_NAME, port_o_K, interfaceObj);
   SM->add_connection(port_ck, clock);

   const auto port_rst = component->find_member(RESET_PORT_NAME, port_o_K, component);
   const auto reset = interfaceObj->find_member(RESET_PORT_NAME, port_o_K, interfaceObj);
   SM->add_connection(port_rst, reset);
}

static void setBusSizes(structural_objectRef component, const HLS_managerRef HLSMgr)
{
   const auto componentModule = GetPointerS<module>(component);
   for(unsigned int i = 0; i < componentModule->get_num_ports(); ++i)
   {
      const auto port = componentModule->get_positional_port(i);
      const auto portObj = GetPointerS<port_o>(port);
      if(portObj->get_is_data_bus())
      {
         portObj->type_resize(HLSMgr->Rmem->get_bus_data_bitsize());
      }
      else if(portObj->get_is_addr_bus())
      {
         portObj->type_resize(HLSMgr->get_address_bitsize());
      }
      else if(portObj->get_is_size_bus())
      {
         portObj->type_resize(HLSMgr->Rmem->get_bus_size_bitsize());
      }
   }
}
