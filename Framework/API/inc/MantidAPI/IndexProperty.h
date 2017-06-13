#ifndef MANTID_API_INDEXPROPERTY_H_
#define MANTID_API_INDEXPROPERTY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/IndexTypeProperty.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace API {

/** IndexProperty : TODO: DESCRIPTION

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL IndexProperty : public Kernel::ArrayProperty<int> {
public:
  explicit IndexProperty(
      const std::string &name, const IWorkspaceProperty &workspaceProp,
      const IndexTypeProperty &indexTypeProp,
      Kernel::IValidator_sptr validator =
          Kernel::IValidator_sptr(new Kernel::NullValidator));

  std::string isValid() const override;
  std::vector<int> &operator=(const std::vector<int> &rhs) override;
  std::string setValue(const std::string &value) override;
  Indexing::SpectrumIndexSet getIndices() const;
  std::string value() const override;

private:
  const IWorkspaceProperty &m_workspaceProp;
  const IndexTypeProperty &m_indexTypeProp;
  int m_min;
  int m_max;
  mutable Indexing::SpectrumIndexSet m_indices;
  mutable bool m_indicesExtracted;
  std::string m_validString;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_INDEXPROPERTY_H_ */