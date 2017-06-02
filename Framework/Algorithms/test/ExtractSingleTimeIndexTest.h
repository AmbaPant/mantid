#ifndef MANTID_ALGORITHMS_EXTRACTSINGLETIMEINDEXTEST_H_
#define MANTID_ALGORITHMS_EXTRACTSINGLETIMEINDEXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ExtractSingleTimeIndex.h"

using Mantid::Algorithms::ExtractSingleTimeIndex;

class ExtractSingleTimeIndexTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractSingleTimeIndexTest *createSuite() { return new ExtractSingleTimeIndexTest(); }
  static void destroySuite( ExtractSingleTimeIndexTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_EXTRACTSINGLETIMEINDEXTEST_H_ */