/**
 * @file vampire.cpp. Implements the top-level procedures of Vampire.
 */

#include <string>
#include <iostream>
#include <ostream>
#include <fstream>
#include <csignal>

#include "Debug/Tracer.hpp"

#include "Lib/Exception.hpp"
#include "Lib/Environment.hpp"
#include "Lib/Int.hpp"
#include "Lib/MapToLIFO.hpp"
#include "Lib/Random.hpp"
#include "Lib/Set.hpp"
#include "Lib/Stack.hpp"
#include "Lib/TimeCounter.hpp"
#include "Lib/Timer.hpp"

#include "Lib/List.hpp"
#include "Lib/Vector.hpp"
#include "Lib/System.hpp"
#include "Lib/Metaiterators.hpp"

#include "Kernel/Clause.hpp"
#include "Kernel/Formula.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Kernel/Problem.hpp"
#include "Kernel/Signature.hpp"
#include "Kernel/Term.hpp"

#include "Indexing/TermSharing.hpp"

#include "Inferences/InferenceEngine.hpp"
#include "Inferences/TautologyDeletionISE.hpp"

#include "InstGen/IGAlgorithm.hpp"

#include "SAT/DIMACS.hpp"

#include "Shell/CASC/CASCMode.hpp"
#include "Shell/CASC/CLTBMode.hpp"
#include "Shell/CASC/SimpleLTBMode.hpp"
#include "Shell/CParser.hpp"
#include "Shell/CommandLine.hpp"
#include "Shell/EqualityProxy.hpp"
#include "Shell/Grounding.hpp"
#include "Shell/Normalisation.hpp"
#include "Shell/Options.hpp"
#include "Shell/Property.hpp"
#include "Saturation/ProvingHelper.hpp"
#include "Shell/Preprocess.hpp"
#include "Shell/Refutation.hpp"
#include "Shell/TheoryFinder.hpp"
#include "Shell/TPTP.hpp"
#include "Parse/TPTP.hpp"
#include "Shell/SpecialTermElimination.hpp"
#include "Shell/Statistics.hpp"
#include "Shell/UIHelper.hpp"

#include "Saturation/SaturationAlgorithm.hpp"


#if CHECK_LEAKS
#include "Lib/MemoryLeak.hpp"
#endif

#define SPIDER 0
#define SAVE_SPIDER_PROPERTIES 0

using namespace Shell;
using namespace SAT;
using namespace Saturation;
using namespace Inferences;
using namespace InstGen;

Problem* globProblem=0;

/**
 * Return value is non-zero unless we were successful.
 *
 * Being successful for modes that involve proving means that we have
 * either found refutation or established satisfiability.
 *
 *
 * If Vampire was interupted by a SIGINT, value
 * VAMP_RESULT_STATUS_SIGINT is returned,
 * and in case of other signal we return VAMP_RESULT_STATUS_OTHER_SIGNAL. For implementation
 * of these return values see Lib/System.hpp.
 *
 * In case of an unhandled exception or user error, we return value
 * VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION.
 *
 * In case Vampire was terminated by the timer, return value is
 * uncertain (but definitely not zero), probably it will be 134
 * (we terminate by a call to the @b abort() function in this case).
 */
int vampireReturnValue = VAMP_RESULT_STATUS_UNKNOWN;

Problem* getPreprocessedProblem()
{
  CALL("getInputClauses");
  
  Problem* prb=UIHelper::getInputProblem();

  TimeCounter tc2(TC_PREPROCESSING);

  Preprocess prepro(*env.options);
  //phases for preprocessing are being set inside the proprocess method
  prepro.preprocess(*prb);
  globProblem=prb;

  return prb;
}

void doProving()
{
  CALL("doProving()");
  ScopedPtr<Problem> prb(getPreprocessedProblem());
  ProvingHelper::runVampireSaturation(*prb, *env.options);
}

/**
 * Read a problem and output profiling information about it.
 * @since 03/08/2008 Torrevieja
 */
void profileMode()
{
  CALL("profileMode()");

  ScopedPtr<Problem> prb(UIHelper::getInputProblem());

  Property* property = prb->getProperty();
  TheoryFinder tf(prb->units(), property);
  Preprocess prepro(*env.options);
  tf.search();

  env.beginOutput();
  env.out() << property->categoryString() << ' '
       << property->props() << ' '
       << property->atoms() << "\n";
  env.endOutput();

  //we have succeeded with the profile mode, so we'll terminate with zero return value
  vampireReturnValue=VAMP_RESULT_STATUS_SUCCESS;
} // profileMode

void programAnalysisMode()
{
  CALL("programAnalysisMode()");

#if 0
  string inputFile = env.options->inputFile();
  istream* input;
  if(inputFile=="") {
    input=&cin;
  } else {
    cout << "Analyzing " << inputFile << "...\n";
    input=new ifstream(inputFile.c_str());
    if(input->fail()) {
      USER_ERROR("Cannot open problem file: "+inputFile);
    }
  }
  string progString("");
  while (!input->eof()) {
    string inp;
    getline(*input,inp);
    progString += inp + '\n';
  }
  // cout << progString;

  CParser parser(progString.c_str());
  parser.tokenize();
  //  parser.output(cout);
#else
  INVALID_OPERATION("program analysis currently not supported");
#endif
  vampireReturnValue=VAMP_RESULT_STATUS_SUCCESS;
} // programAnalysisMode

void vampireMode()
{
  CALL("vampireMode()");

  if(env.options->mode()==Options::MODE_CONSEQUENCE_ELIMINATION) {
    env.options->setUnusedPredicateDefinitionRemoval(false);
    env.options->setPropositionalToBDD(false);
  }

  string inputFile = env.options->inputFile();
  istream* input;
  if(inputFile=="") {
    input=&cin;
  }
  else {
    input=new ifstream(inputFile.c_str());
    if(input->fail()) {
      USER_ERROR("Cannot open problem file: "+inputFile);
    }
  }

  doProving();

  env.beginOutput();
  UIHelper::outputResult(env.out());
  env.endOutput();

#if SATISFIABLE_IS_SUCCESS
  if(env.statistics->terminationReason==Statistics::REFUTATION ||
      env.statistics->terminationReason==Statistics::SATISFIABLE) {
#else
    if(env.statistics->terminationReason==Statistics::REFUTATION) {
#endif
    vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
  }
} // vampireMode


void spiderMode()
{
  CALL("spiderMode()");
  bool noException=true;
  try {
    doProving();
  }
  catch(...) {
    noException=false;
  }

  env.beginOutput();
  if(noException) {
    switch (env.statistics->terminationReason) {
    case Statistics::REFUTATION:
      reportSpiderStatus('+');
      vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      break;
    case Statistics::TIME_LIMIT:
    case Statistics::MEMORY_LIMIT:
    case Statistics::UNKNOWN:
    case Statistics::REFUTATION_NOT_FOUND:
      reportSpiderStatus('?');
      break;
    case Statistics::SATISFIABLE:
      reportSpiderStatus('-');
#if SATISFIABLE_IS_SUCCESS
      vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
#endif
      break;
    default:
      ASSERTION_VIOLATION;
    }
    env.statistics->print(env.out());
  }
  else {
    reportSpiderFail();
  }
  env.endOutput();
} // spiderMode

void clausifyMode()
{
  CALL("clausifyMode()");

  CompositeISE simplifier;
  simplifier.addFront(new TrivialInequalitiesRemovalISE());
  simplifier.addFront(new TautologyDeletionISE());
  simplifier.addFront(new DuplicateLiteralRemovalISE());

  ScopedPtr<Problem> prb(getPreprocessedProblem());
  ClauseIterator cit = prb->clauseIterator();
  env.beginOutput();
  while (cit.hasNext()) {
    Clause* cl=cit.next();
    cl=simplifier.simplify(cl);
    if(!cl) {
      continue;
    }
    env.out() << TPTP::toString(cl) << "\n";
  }
  env.endOutput();

  //we have successfully output all clauses, so we'll terminate with zero return value
  vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
} // clausifyMode

void axiomSelectionMode()
{
  CALL("axiomSelectionMode()");

  env.options->setSineSelection(Options::SS_AXIOMS);

  ScopedPtr<Problem> prb(UIHelper::getInputProblem());

  if(prb->hasSpecialTermsOrLets()) {
    SpecialTermElimination().apply(*prb);
  }

  // reorder units
  if (env.options->normalize()) {
    env.statistics->phase=Statistics::NORMALIZATION;
    Normalisation norm;
    norm.normalise(*prb);
  }

  env.statistics->phase=Statistics::SINE_SELECTION;
  SineSelector(*env.options).perform(*prb);

  env.statistics->phase=Statistics::FINALIZATION;

  UnitList::Iterator uit(prb->units());
  env.beginOutput();
  while (uit.hasNext()) {
    Unit* u=uit.next();
    env.out() << TPTP::toString(u) << "\n";
  }
  env.endOutput();

  //we have successfully output the selected units, so we'll terminate with zero return value
  vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
}

void groundingMode()
{
  CALL("groundingMode()");

  try {
    ScopedPtr<Problem> prb(UIHelper::getInputProblem());

    Preprocess prepro(*env.options);
    prepro.preprocess(*prb);

    ClauseIterator clauses=prb->clauseIterator();

    if(prb->hasEquality()) {
      ClauseList* eqAxioms=Grounding::getEqualityAxioms(prb->getProperty()->positiveEqualityAtoms()!=0);
      clauses=pvi(getConcatenatedIterator(ClauseList::DestructiveIterator(eqAxioms),clauses));
    }

    MapToLIFO<Clause*, SATClause*> insts;
    Grounding gnd;
    SATClause::NamingContext nameCtx;

    while(clauses.hasNext()) {
      Clause* cl=clauses.next();
      ClauseList* grounded=gnd.ground(cl);
      SATClauseList* sGrounded=0;
      while(grounded) {
	Clause* gcl=ClauseList::pop(grounded);
	SATClauseList::push(SATClause::fromFOClause(nameCtx, gcl), sGrounded);
      }
      insts.pushManyToKey(cl, sGrounded);
    }
    env.beginOutput();
    DIMACS::outputGroundedProblem(insts, nameCtx, env.out());
    env.endOutput();

  } catch(MemoryLimitExceededException) {
    env.beginOutput();
    env.out()<<"Memory limit exceeded\n";
    env.endOutput();
  } catch(TimeLimitExceededException) {
    env.beginOutput();
    env.out()<<"Time limit exceeded\n";
    env.endOutput();
  }
} // groundingMode

void explainException (Exception& exception)
{
  env.beginOutput();
  exception.cry(env.out());
  env.endOutput();
} // explainException

/**
 * The main function.
  * @since 03/12/2003 many changes related to logging
  *        and exception handling.
  * @since 10/09/2004, Manchester changed to use knowledge bases
  */
int main(int argc, char* argv [])
{
  CALL ("main");

  System::registerArgv0(argv[0]);
  System::setSignalHandlers();
   // create random seed for the random number generation
  Lib::Random::setSeed(123456);

  try {
    // read the command line and interpret it
    Shell::CommandLine cl(argc,argv);
    cl.interpret(*env.options);

    PROCESS_TRACE_SPEC_STRING(env.options->traceSpecString());
    env.options->enableTracesAccordingToOptions();

    if(env.options->showOptions()) {
      env.beginOutput();
      env.options->output(env.out());
      env.endOutput();
    }

    Allocator::setMemoryLimit(env.options->memoryLimit()*1048576ul);
    Lib::Random::setSeed(env.options->randomSeed());

    switch (env.options->mode())
    {
    case Options::MODE_AXIOM_SELECTION:
      axiomSelectionMode();
      break;
    case Options::MODE_GROUNDING:
      groundingMode();
      break;
    case Options::MODE_SPIDER:
      spiderMode();
      break;
    case Options::MODE_CONSEQUENCE_ELIMINATION:
    case Options::MODE_VAMPIRE:
      vampireMode();
      break;
    case Options::MODE_CASC:
      if(Shell::CASC::CASCMode::perform(argc, argv)) {
	//casc mode succeeded in solving the problem, so we return zero
	vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      }
      break;
    case Options::MODE_CASC_SIMPLE_LTB:
    {
      Shell::CASC::SimpleLTBMode sltbm;
      sltbm.perform();
      //we have processed the ltb batch file, so we can return zero
      vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      break;
    }
    case Options::MODE_CASC_LTB:
    {
      Shell::CASC::CLTBMode::perform();
      //we have processed the ltb batch file, so we can return zero
      vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      break;
    }
    case Options::MODE_CLAUSIFY:
      clausifyMode();
      break;
    case Options::MODE_PROFILE:
      profileMode();
      break;
    case Options::MODE_PROGRAM_ANALYSIS:
      programAnalysisMode();
      break;
    default:
      USER_ERROR("Unsupported mode");
    }
#if CHECK_LEAKS
    if (globUnitList) {
      MemoryLeak leak;
      leak.release(globUnitList);
    }
    delete env.signature;
    env.signature = 0;
#endif
  }
#if VDEBUG
  catch (Debug::AssertionViolationException& exception) {
    vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;
    reportSpiderFail();
#if CHECK_LEAKS
    MemoryLeak::cancelReport();
#endif
  }
#endif
  catch (UserErrorException& exception) {
    vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;
    reportSpiderFail();
#if CHECK_LEAKS
    MemoryLeak::cancelReport();
#endif
    explainException(exception);
  }
  catch (Exception& exception) {
    vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;
    reportSpiderFail();
#if CHECK_LEAKS
    MemoryLeak::cancelReport();
#endif
    env.beginOutput();
    explainException(exception);
    env.statistics->print(env.out());
    env.endOutput();
  }
  catch (std::bad_alloc& _) {
    vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;
    reportSpiderFail();
#if CHECK_LEAKS
    MemoryLeak::cancelReport();
#endif
    env.beginOutput();
    env.out() << "Insufficient system memory" << '\n';
    env.endOutput();
  }
//   delete env.allocator;

  return vampireReturnValue;
} // main

