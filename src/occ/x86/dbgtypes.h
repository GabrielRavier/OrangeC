/* Software License Agreement
 *
 *     Copyright(C) 1994-2018 David Lindauer, (LADSoft)
 *
 *     This file is part of the Orange C Compiler package.
 *
 *     The Orange C Compiler package is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version, with the addition of the
 *     Orange C "Target Code" exception.
 *
 *     The Orange C Compiler package is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
 *
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 *
 */

#define DEBUG_VERSION 4.0
#include "ObjType.h"

class ObjFactory;
class ObjFile;

#include <map>

class dbgtypes
{
  public:
    dbgtypes(ObjFactory& Factory, ObjFile* FI) : factory(Factory), fi(FI) {}
    ObjType* Put(TYPE* tp);
    void OutputTypedef(SYMBOL* sp);

  protected:
    ObjType* Lookup(TYPE* tp);
    ObjType* BasicType(TYPE* tp);
    ObjType* TypeName(ObjType* val, const char* nm);
    void StructFields(ObjType::eType sel, ObjType* val, int sz, SYMBOL* parent, HASHREC* hr);
    void EnumFields(ObjType* val, ObjType* base, int sz, HASHREC* hr);
    ObjType* Function(TYPE* tp);
    ObjType* ExtendedType(TYPE* tp);

  private:
    struct typecompare
    {
        bool operator()(const TYPE* left, const TYPE* right) const;
    };
    std::map<TYPE*, ObjType*, typecompare> hash;
    ObjFactory& factory;
    ObjFile* fi;
};
