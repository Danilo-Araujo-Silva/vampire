
/*
 * File IntegerSet.hpp.
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
 * @file IntegerSet.hpp
 * Defines class IntegerSet for sets of nonnegative integers.
 *
 * @since 16/06/2005 Manchester
 */

#ifndef __IntegerSet__
#define __IntegerSet__

#define BITS_PER_INT (8*sizeof(unsigned int))

namespace Lib {

/**
 * Class IntegerSet for sets of nonnegative integers.
 *
 * @since 16/06/2005 Manchester
 */
class IntegerSet
{
public:
  IntegerSet() : _words(0), _set(0) {}
  ~IntegerSet();
  void insert(int n);
  void remove(int n);
  bool member(int n) const;
private:
  /** the number of words used by this set */
  int _words;
  /** the set itself coded as an array of unsigned integers */
  unsigned int* _set;
}; // class IntegerSet

} // namespace Lib

#endif
