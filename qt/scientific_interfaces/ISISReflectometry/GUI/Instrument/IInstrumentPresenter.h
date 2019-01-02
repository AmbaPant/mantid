// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IINSTRUMENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IINSTRUMENTPRESENTER_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IBatchPresenter;

/** @class IInstrumentPresenter

IInstrumentPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Instrument' presenter
*/
class IInstrumentPresenter {
public:
  virtual ~IInstrumentPresenter() = default;

  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual void reductionPaused() = 0;
  virtual void reductionResumed() = 0;
  virtual void autoreductionPaused() = 0;
  virtual void autoreductionResumed() = 0;
  virtual void instrumentChanged(std::string const &instrumentName) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IINSTRUMENTPRESENTER_H */
