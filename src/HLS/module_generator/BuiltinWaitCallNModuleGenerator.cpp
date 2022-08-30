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
 *              Copyright (C) 2022-2022 Politecnico di Milano
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
 * @file BuiltinWaitCallNModuleGenerator.cpp
 * @brief
 *
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "BuiltinWaitCallNModuleGenerator.hpp"

#include "application_manager.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "op_graph.hpp"
#include "structural_objects.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

BuiltinWaitCallNModuleGenerator::BuiltinWaitCallNModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void BuiltinWaitCallNModuleGenerator::InternalExec(std::ostream& out, const module* /* mod */, unsigned int function_id,
                                                   vertex op_v, const HDLWriter_Language /* language */,
                                                   const std::vector<ModuleGenerator::parameter>& _p,
                                                   const std::vector<ModuleGenerator::parameter>& /* _ports_in */,
                                                   const std::vector<ModuleGenerator::parameter>& /* _ports_out */,
                                                   const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   const auto retval_size = [&]() {
      THROW_ASSERT(function_id && op_v, "");
      const auto FB = HLSMgr->CGetFunctionBehavior(function_id);
      const auto TM = HLSMgr->get_tree_manager();
      const auto call_stmt =
          TM->CGetTreeNode(FB->CGetOpGraph(FunctionBehavior::CFG)->CGetOpNodeInfo(op_v)->GetNodeId());
      THROW_ASSERT(call_stmt && call_stmt->get_kind() == gimple_call_K, "Expected gimple call statement.");
      const auto gc = GetPointerS<const gimple_call>(call_stmt);
      THROW_ASSERT(gc->args.size() >= 2, "Expected at least two arguments for the builtin wait call.");
      const auto called_addr = gc->args.at(0);
      const auto called_hasreturn = gc->args.at(1);
      THROW_ASSERT(GET_CONST_NODE(called_hasreturn)->get_kind() == integer_cst_K, "");
      const auto hasreturn =
          tree_helper::get_integer_cst_value(GetPointerS<const integer_cst>(GET_CONST_NODE(called_hasreturn)));
      if(hasreturn)
      {
         const auto fpointer_type = tree_helper::CGetType(called_addr);
         const auto called_ftype = tree_helper::CGetPointedType(fpointer_type);
         const auto return_type = tree_helper::GetFunctionReturnType(called_ftype);
         if(return_type)
         {
            return tree_helper::Size(return_type);
         }
      }
      return 0U;
   }();

   // Signals declarations
   if(_p.size() == 3U)
   {
      out << "reg [0:0] index;\n\n";
   }
   else if(_p.size() > 3U)
   {
      out << "reg [" << ceil_log2(_p.size() - 2U) << "-1:0] index;\n\n";
   }

   if(_p.size() > 2U)
   {
      out << "wire [BITSIZE_Mout_addr_ram-1:0] paramAddressRead;\n\n";
   }

   out << "reg [31:0] step 1INIT_ZERO_VALUE;\n"
       << "reg [31:0] next_step;\n"
       << "reg done_port;\n"
       << "reg [PORTSIZE_Sout_DataRdy-1:0] Sout_DataRdy;\n"
       << "reg [PORTSIZE_Mout_oe_ram-1:0] Mout_oe_ram;\n"
       << "reg [PORTSIZE_Mout_we_ram-1:0] Mout_we_ram;\n"
       << "reg [PORTSIZE_Mout_addr_ram*BITSIZE_Mout_addr_ram-1:0] Mout_addr_ram;\n"
       << "reg [PORTSIZE_Mout_Wdata_ram*BITSIZE_Mout_Wdata_ram-1:0] Mout_Wdata_ram;\n"
       << "reg [PORTSIZE_Mout_data_ram_size*BITSIZE_Mout_data_ram_size-1:0] Mout_data_ram_size;\n\n";

   if(retval_size)
   {
      out << "reg [" << retval_size << "-1:0] readValue 1INIT_ZERO_VALUE;\n"
          << "reg [" << retval_size << "-1:0] next_readValue;\n\n";
   }

   if(_p.size() > 2U)
   {
      out << "reg [BITSIZE_Mout_addr_ram-1:0] paramAddress [" << (_p.size() - 2U) << "-1:0];\n\n";
   }

   out << "function [PORTSIZE_S_addr_ram-1:0] check_condition;\n"
       << "  input [PORTSIZE_S_addr_ram*BITSIZE_S_addr_ram-1:0] m;\n"
       << "  integer i1;\n"
       << "  begin\n"
       << "    for(i1 = 0; i1 < PORTSIZE_S_addr_ram; i1 = i1 + 1)\n"
       << "    begin\n"
       << "      check_condition[i1] = m[i1*BITSIZE_S_addr_ram +:BITSIZE_S_addr_ram] == unlock_address;\n"
       << "    end\n"
       << "  end\n"
       << "endfunction\n";

   out << "wire [PORTSIZE_S_addr_ram-1:0] internal;\n";

   out << "parameter [31:0] ";
   const auto n_iterations = retval_size ? (_p.size() + 3U) : _p.size();
   for(auto idx = 0U; idx <= n_iterations; ++idx)
   {
      if(idx != n_iterations)
      {
         out << "S_" << idx << " = 32'd" << idx << ",\n";
      }
      else
      {
         out << "S_" << idx << " = 32'd" << idx << ";\n";
      }
   }

   if(_p.size() > 2U)
   {
      out << "initial\n"
          << "   begin\n"
          << "     $readmemb(MEMORY_INIT_file, paramAddress, 0, " << (_p.size() - 2U) << "-1);\n"
          << "   end\n\n\n";
   }

   if(_p.size() > 2U)
   {
      out << "assign paramAddressRead = paramAddress[index];\n";
   }
   out << "assign Sout_Rdata_ram = Sin_Rdata_ram;\n"
       << "assign internal = check_condition(S_addr_ram);\n";

   // State machine
   out << "always @ (posedge clock 1RESET_EDGE)\n"
       << "  if (1RESET_VALUE)\n"
       << "  begin\n"
       << "    step <= 0;\n";

   if(retval_size)
   {
      if(retval_size == 1U)
      {
         out << "    readValue <= {1'b0};\n";
      }
      else
      {
         out << "    readValue <= {" << retval_size << " {1'b0}};\n";
      }
      out << "  end else begin\n"
          << "    step <= next_step;\n"
          << "    readValue <= next_readValue;\n"
          << "  end\n\n";
   }
   else
   {
      out << "  end else begin\n"
          << "    step <= next_step;\n"
          << "  end\n\n";
   }

   if(_p.size() > 2U)
   {
      out << "always @(*)\n"
          << "  begin\n"
          << "    index = 0;\n"
          << "    if (step == S_0) begin\n"
          << "        index = 0;\n"
          << "    end\n";
   }

   auto idx = 1U;
   if(_p.size() > 3)
   {
      for(idx = 1U; idx <= (_p.size() - 3U); ++idx)
      {
         out << "     else if (step == S_" << idx << ") begin\n"
             << "       index = " << (idx - 1U) << ";\n"
             << "     end\n";
      }
   }
   if(_p.size() > 2U)
   {
      out << "    else if (step == S_" << idx << ") begin\n"
          << "      index = " << (idx - 1U) << ";\n"
          << "    end\n";
      idx++;
   }

   idx++;

   idx++;

   if(_p.size() > 2U && retval_size)
   {
      out << "  else if (step == S_" << idx << ") begin\n"
          << "    index = " << (idx - 4U) << ";\n"
          << "  end\n";
      idx++;
   }
   if(_p.size() > 2U)
   {
      out << "end\n";
   }

   out << "always @(*)\n"
       << "  begin\n"
       << "  Sout_DataRdy = Sin_DataRdy;\n"
       << "  done_port = 1'b0;\n"
       << "  next_step = S_0;\n"
       << (retval_size ? "  next_readValue = readValue;\n" : "") << "  Mout_we_ram = Min_we_ram;\n"
       << "  Mout_Wdata_ram = Min_Wdata_ram;\n"
       << "  Mout_oe_ram = Min_oe_ram;\n"
       << "  Mout_addr_ram = Min_addr_ram;\n"
       << "  Mout_data_ram_size = Min_data_ram_size;\n";

   out << "  if (step == S_0) begin\n"
       << "    if (start_port == 1'b1) begin\n";
   if(_p.size() == 3U)
   {
      out << "      next_step = in2[0] ? S_2 : S_1;\n";
   }
   else
   {
      out << "      next_step = S_1;\n";
   }
   out << "    end else begin\n"
       << "      next_step = S_0;\n"
       << "    end\n"
       << "  end\n";
   idx = 1U;
   if(_p.size() > 3)
   {
      for(idx = 1U; idx <= (_p.size() - 3U); ++idx)
      {
         if(idx != (_p.size() - 3U))
         {
            out << "  else if (step == S_" << idx << ") begin\n"
                << "    Mout_we_ram[0] = 1'b1;\n"
                << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1 + paramAddressRead;\n"
                << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = " << _p[idx + 1U].name << ";\n"
                << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << _p[idx + 1U].type_size << ";\n"
                << "    if (M_DataRdy[0] == 1'b1) begin\n"
                << "      next_step = S_" << (idx + 1U) << ";\n"
                << "    end else begin\n"
                << "      next_step = S_" << idx << ";\n"
                << "    end\n"
                << "  end\n";
         }
         else
         {
            out << "  else if (step == S_" << idx << ") begin\n"
                << "    Mout_we_ram[0] = 1'b1;\n"
                << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1 + paramAddressRead;\n"
                << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = " << _p[idx + 1U].name << ";\n"
                << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << _p[idx + 1U].type_size << ";\n"
                << "    if (M_DataRdy[0] == 1'b1) begin\n"
                << "      next_step = in2[0] ? S_" << (idx + 2U) << " : S_" << (idx + 1U) << ";\n"
                << "    end else begin\n"
                << "      next_step = S_" << idx << ";\n"
                << "    end\n"
                << "  end\n";
         }
      }
   }
   if(_p.size() > 2U)
   {
      out << "  else if (step == S_" << idx << ") begin\n"
          << "     Mout_we_ram[0] = 1'b1;\n"
          << "     Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1 + paramAddressRead;\n"
          << "     Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = " << _p[idx + 1U].name << ";\n"
          << "     Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << _p[idx + 1U].type_size << ";\n"
          << "   if (M_DataRdy[0] == 1'b1) begin\n"
          << "     next_step = S_" << (idx + 1U) << ";\n"
          << "   end else begin\n"
          << "     next_step = S_" << idx << ";\n"
          << "   end\n"
          << "  end\n";
      idx++;
   }

   out << "  else if (step == S_" << idx << ") begin\n"
       << "    Mout_we_ram[0] = 1'b1;\n"
       << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1;\n"
       << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = unlock_address;\n"
       << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = 32;\n"
       << "    if (M_DataRdy[0] == 1'b1) begin\n"
       << "      next_step = S_" << (idx + 1U) << ";\n"
       << "    end else begin\n"
       << "      next_step = S_" << idx << ";\n"
       << "    end"
       << "  end\n";
   idx++;

   out << "  else if (step == S_" << idx << ") begin\n"
       << "    if (|(S_we_ram & internal)) begin\n"
       << "      Sout_DataRdy = (S_we_ram & internal) | Sin_DataRdy;\n"
       << "      next_step = in2[0] ? S_" << (retval_size ? (idx + 1U) : 0U) << " : S_0;\n"
       << "      done_port = in2[0] ? 1'b0 : 1'b1;\n"
       << "    end else begin\n"
       << "      next_step = S_" << idx << ";\n"
       << "    end\n"
       << "  end\n";
   idx++;

   if(_p.size() > 2U && retval_size)
   {
      out << "  else if (step == S_" << idx << ") begin\n"
          << "      Mout_oe_ram[0] = 1'b1;\n"
          << "      Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = in1 + paramAddressRead ;\n"
          << "      Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << retval_size << ";\n"
          << "    if (M_DataRdy[0] == 1'b1) begin\n"
          << "      next_step = S_" << (idx + 1U) << ";\n"
          << "      next_readValue = M_Rdata_ram;\n"
          << "    end else begin\n"
          << "      next_step = S_" << idx << ";\n"
          << "    end"
          << "  end\n";
      idx++;

      out << "  else if (step == S_" << idx << ") begin\n"
          << "    Mout_we_ram[0] = 1'b1;\n"
          << "    Mout_addr_ram[BITSIZE_Mout_addr_ram-1:0] = " << _p[_p.size() - 1U].name << ";\n"
          << "    Mout_Wdata_ram[BITSIZE_Mout_Wdata_ram-1:0] = readValue;\n"
          << "    Mout_data_ram_size[BITSIZE_Mout_data_ram_size-1:0] = " << retval_size << ";\n"
          << "    if (M_DataRdy[0] == 1'b1) begin\n"
          << "      next_step = S_0;\n"
          << "      done_port = 1'b1;\n"
          << "    end else begin\n"
          << "      next_step = S_" << idx << ";\n"
          << "    end"
          << "  end\n";
   }
   out << "end\n";
}