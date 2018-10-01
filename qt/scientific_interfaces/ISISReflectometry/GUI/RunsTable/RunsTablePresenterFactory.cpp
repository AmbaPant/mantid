#include "RunsTablePresenterFactory.h"
#include "MantidKernel/make_unique.h"

namespace MantidQt {
namespace CustomInterfaces {

RunsTablePresenterFactory::RunsTablePresenterFactory(
    std::vector<std::string> const &instruments, double thetaTolerance)
    : m_instruments(instruments), m_thetaTolerance(thetaTolerance) {}

std::unique_ptr<RunsTablePresenter> RunsTablePresenterFactory::
operator()(IRunsTableView *view) const {
  return Mantid::Kernel::make_unique<RunsTablePresenter>(
      view, m_instruments, m_thetaTolerance, Jobs());
}
} // namespace CustomInterfaces
} // namespace MantidQt
