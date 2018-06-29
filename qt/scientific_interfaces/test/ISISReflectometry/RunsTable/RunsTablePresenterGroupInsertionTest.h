#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPINSERTTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPINSERTTEST_H_

#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "RunsTablePresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;
using testing::Mock;
using testing::NiceMock;

class RunsTablePresenterGroupInsertionTest : public CxxTest::TestSuite,
                                             RunsTablePresenterTest {
public:
  static RunsTablePresenterGroupInsertionTest *createSuite() {
    return new RunsTablePresenterGroupInsertionTest();
  }

  static void destroySuite(RunsTablePresenterGroupInsertionTest *suite) {
    delete suite;
  }

  void testExpandsAllGroupsWhenRequested() {
    EXPECT_CALL(m_jobs, expandAll());

    auto presenter = makePresenter(m_view);
    presenter.notifyExpandAllRequested();

    verifyAndClearExpectations();
  }

  void testCollapsesAllGroupsWhenRequested() {
    EXPECT_CALL(m_jobs, collapseAll());

    auto presenter = makePresenter(m_view);
    presenter.notifyCollapseAllRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesViewWhenGroupInsertedAfterSelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {location(0)});
    EXPECT_CALL(m_jobs, insertChildRowOf(location(), 1))
        .WillOnce(Return(location(1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenGroupInsertedAfterSelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {location(0)});
    ON_CALL(m_jobs, insertChildRowOf(location(), 1))
        .WillByDefault(Return(location(1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();

    TS_ASSERT_EQUALS(3, groups.size());
    TS_ASSERT_EQUALS("", groups[1].name());

    verifyAndClearExpectations();
  }

  void testUpdatesViewWhenGroupAppendedBasedOnEmptySelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {});
    EXPECT_CALL(m_jobs, appendChildRowOf(location()))
        .WillOnce(Return(location(2)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    verifyAndClearExpectations();
  }

  void testUpdatesModelWhenGroupAppendedBasedOnEmptySelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {});
    ON_CALL(m_jobs, insertChildRowOf(location(), 1))
        .WillByDefault(Return(location(1)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();

    TS_ASSERT_EQUALS(3, groups.size());
    TS_ASSERT_EQUALS("", groups[2].name());
    verifyAndClearExpectations();
  }

  void testInsertsGroupAfterLastSelectedBasedOnMultiSelection() {
    auto reductionJobs = twoEmptyGroupsModel();

    selectedRowLocationsAre(m_jobs, {location(1), location(0)});
    ON_CALL(m_jobs, insertChildRowOf(location(), 2))
        .WillByDefault(Return(location(2)));

    auto presenter = makePresenter(m_view, std::move(reductionJobs));
    presenter.notifyInsertGroupRequested();

    auto &groups = unslicedJobsFromPresenter(presenter).groups();
    TS_ASSERT_EQUALS(3, groups.size());
    TS_ASSERT_EQUALS("", groups[2].name());

    verifyAndClearExpectations();
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERGROUPINSERTTEST_H_
