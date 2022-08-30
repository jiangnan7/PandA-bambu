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
 * @file Write_noneDSModuleGenerator.cpp
 * @brief
 *
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "Write_noneDSModuleGenerator.hpp"

#include "language_writer.hpp"

Write_noneDSModuleGenerator::Write_noneDSModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void Write_noneDSModuleGenerator::InternalExec(std::ostream& out, const module* /* mod */,
                                               unsigned int /* function_id */, vertex /* op_v */,
                                               const HDLWriter_Language /* language */,
                                               const std::vector<ModuleGenerator::parameter>& /* _p */,
                                               const std::vector<ModuleGenerator::parameter>& _ports_in,
                                               const std::vector<ModuleGenerator::parameter>& _ports_out,
                                               const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   out << "integer ii=0;\n";
   out << "reg [" << _ports_out[0].type_size << "-1:0] reg_" << _ports_out[0].name << ";\n";
   out << "assign " << _ports_out[0].name << " = reg_" << _ports_out[0].name << ";\n";
   out << "always @(*)\n";
   out << "begin\n";
   out << "  reg_" << _ports_out[0].name << " = 0;\n";
   out << "  for(ii=0; ii<PORTSIZE_" << _ports_in[1].name << "; ii=ii+1)\n";
   out << "  begin\n";
   out << "    reg_" << _ports_out[0].name << " = (" << _ports_in[1].name << "[(BITSIZE_" << _ports_in[1].name
       << ")*ii+:BITSIZE_" << _ports_in[1].name << "]>=" << _ports_out[0].type_size << ")?" << _ports_in[2].name
       << "[(BITSIZE_" << _ports_in[2].name << ")*ii+:BITSIZE_" << _ports_in[2].name << "]:(reg_" << _ports_out[0].name
       << "^((((BITSIZE_" << _ports_in[2].name << ">=" << _ports_out[0].type_size << "?" << _ports_in[2].name
       << "[(BITSIZE_" << _ports_in[2].name << ")*ii+:BITSIZE_" << _ports_in[2].name << "]:{{("
       << _ports_out[0].type_size << "<BITSIZE_" << _ports_in[2].name << " ? 1 : " << _ports_out[0].type_size
       << "-BITSIZE_" << _ports_in[2].name << "){1'b0}}," << _ports_in[2].name << "[(BITSIZE_" << _ports_in[2].name
       << ")*ii+:BITSIZE_" << _ports_in[2].name << "]})<<" << _ports_in[3].name << "[(BITSIZE_" << _ports_in[3].name
       << ")*ii+:BITSIZE_" << _ports_in[3].name << "]*8)^reg_" << _ports_out[0].name << ") & (((" << _ports_in[1].name
       << "[(BITSIZE_" << _ports_in[1].name << ")*ii+:BITSIZE_" << _ports_in[1].name << "]+" << _ports_in[3].name
       << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_" << _ports_in[3].name << "]*8)>"
       << _ports_out[0].type_size << ") ? ((({(" << _ports_out[0].type_size << "){1'b1}})>>(" << _ports_in[3].name
       << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_" << _ports_in[3].name << "]*8))<<(" << _ports_in[3].name
       << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_" << _ports_in[3].name << "]*8)) : ((((({("
       << _ports_out[0].type_size << "){1'b1}})>>(" << _ports_in[3].name << "[(BITSIZE_" << _ports_in[3].name
       << ")*ii+:BITSIZE_" << _ports_in[3].name << "]*8))<<(" << _ports_in[3].name << "[(BITSIZE_" << _ports_in[3].name
       << ")*ii+:BITSIZE_" << _ports_in[3].name << "]*8))<<(" << _ports_out[0].type_size << "-" << _ports_in[1].name
       << "[(BITSIZE_" << _ports_in[1].name << ")*ii+:BITSIZE_" << _ports_in[1].name << "]-" << _ports_in[3].name
       << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_" << _ports_in[3].name << "]*8))>>("
       << _ports_out[0].type_size << "-" << _ports_in[1].name << "[(BITSIZE_" << _ports_in[1].name << ")*ii+:BITSIZE_"
       << _ports_in[1].name << "]-" << _ports_in[3].name << "[(BITSIZE_" << _ports_in[3].name << ")*ii+:BITSIZE_"
       << _ports_in[3].name << "]*8)))));\n";
   out << "  end\n";
   out << "end\n";
}