// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidICat/ICat3/ICat3ErrorHandling.h"
#include "MantidICat/ICat3/GSoapGenerated/ICat3ICATPortBindingProxy.h"

namespace Mantid::ICat {

/**This method throws the error string returned by gsoap to mantid upper layer
 *@param icat :: -ICat proxy object
 */
void CErrorHandling::throwErrorMessages(ICat3::ICATPortBindingProxy &icat) {
  char buf[600];
  const int len = 600;
  icat.soap_sprint_fault(buf, len);
  std::string error(buf);
  std::string begmsg("<message>");
  std::string endmsg("</message>");

  std::basic_string<char>::size_type index1 = error.find(begmsg);
  std::basic_string<char>::size_type index2 = error.find(endmsg);
  std::string exception;
  if (index1 != std::string::npos && index2 != std::string::npos) {
    exception = error.substr(index1 + begmsg.length(), index2 - (index1 + begmsg.length()));
  }
  throw std::runtime_error(exception);
}
/////////////////////

/// constructor
SessionException::SessionException(const std::string &error) : std::runtime_error(error), m_error(error) {}

const char *SessionException::what() const noexcept { return m_error.c_str(); }
} // namespace Mantid::ICat
