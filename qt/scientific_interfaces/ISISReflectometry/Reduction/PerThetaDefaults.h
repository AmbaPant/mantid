#ifndef MANTID_CUSTOMINTERFACES_PERTHETADEFAULTS_H_
#define MANTID_CUSTOMINTERFACES_PERTHETADEFAULTS_H_
#include "../DllConfig.h"
#include "RangeInQ.h"
#include "ReductionOptionsMap.h"
#include <boost/optional.hpp>
#include <vector>
namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL PerThetaDefaults {
public:
  PerThetaDefaults(boost::optional<double> theta,
                   std::pair<std::string, std::string> tranmissionRuns,
                   boost::optional<RangeInQ> qRange,
                   boost::optional<double> scaleFactor,
                   ReductionOptionsMap reductionOptions);

  std::pair<std::string, std::string> const &transmissionWorkspaceNames() const;
  bool isWildcard() const;
  boost::optional<double> thetaOrWildcard() const;
  boost::optional<RangeInQ> const &qRange() const;
  boost::optional<double> scaleFactor() const;
  ReductionOptionsMap const &reductionOptions() const;

private:
  boost::optional<double> m_theta;
  std::pair<std::string, std::string> m_transmissionRuns;
  boost::optional<RangeInQ> m_qRange;
  boost::optional<double> m_scaleFactor;
  ReductionOptionsMap m_reductionOptions;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_PERTHETADEFAULTS_H_
