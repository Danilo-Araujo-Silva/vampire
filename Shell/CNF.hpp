
/*
 * File CNF.hpp.
 *
 * This file is part of the source code of the software program
 * Vampire 4.2.2. It is protected by applicable
 * copyright laws.
 *
 * This source code is distributed under the licence found here
 * https://vprover.github.io/license.html
 * and in the source directory
 *
 * In summary, you are allowed to use Vampire for non-commercial
 * uses but not allowed to distribute, modify, copy, create derivatives,
 * or use in competitions. 
 * For other uses of Vampire please contact developers for a different
 * licence, which we will make an effort to provide. 
 */
/**
 * @file CNF.hpp
 * Defines class CNF implementing CNF transformation.
 * @since 19/01/2004 Manchester
 * @since 27/12/2007 Manchester, changed completely to a new implementation
 */

#ifndef __CNF__
#define __CNF__

#include "Lib/Stack.hpp"

namespace Kernel {
  class Formula;
  class FormulaUnit;
  class Clause;
  class Unit;
  class Literal;
};

using namespace Lib;
using namespace Kernel;

namespace Shell {

/**
 * Class implementing the CNF transformation.
 * @since 19/01/2004 Manchester
 */
class CNF
{
public:
  CNF();
  void clausify (Unit*,Stack<Clause*>& stack);
private:
  void clausify(Formula*);
  /** The unit currently being processed */
  FormulaUnit* _unit;
  /** stack to collect the results */
  Stack<Clause*>* _result;
  /** stack of literals collected so far */
  Stack <Literal*> _literals;
  /** stack of formulas  */
  Stack <Formula*> _formulas;
}; // class CNF

}
#endif
