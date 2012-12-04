// Copyright (C) 2012 Chris N. Richardson
//
// This file is part of DOLFIN.
//
// DOLFIN is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DOLFIN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.
//
// Modified by Garth N. Wells, 2012
//
// First added:  2012-05-22
// Last changed: 2012-11-27

#ifndef __DOLFIN_HDF5FILE_H
#define __DOLFIN_HDF5FILE_H

#ifdef HAS_HDF5

#include <string>
#include <utility>
#include <vector>
#include "dolfin/common/Timer.h"
#include "dolfin/common/types.h"
#include "dolfin/common/Variable.h"
#include "dolfin/mesh/Mesh.h"
#include "dolfin/mesh/MeshEditor.h"
#include "dolfin/mesh/Vertex.h"
#include "HDF5Interface.h"

namespace dolfin
{

  class Function;
  class GenericVector;

  class HDF5File : public Variable
  {
  public:

    /// Constructor
    HDF5File(const std::string filename, bool truncate, bool use_mpiio=true);

    /// Destructor
    ~HDF5File();

    /// Write Vector to file in a format suitable for re-reading
    void write(const GenericVector& x, const std::string name);

    /// Write Mesh to file
    void write(const Mesh& mesh, const std::string name);

    /// Write Mesh of given cell dimension to file in a format suitable
    /// for re-reading
    void write(const Mesh& mesh, const std::size_t cell_dim, const std::string name);

    /// Write Mesh to file for visualisation (may contain duplicate
    /// entities and will not preserve global indices)
    void write_visualisation(const Mesh& mesh, const std::string name);

    /// Write Mesh of given cell dimension to file for visualisation (may
    /// contain duplicate entities and will not preserve global indices)
    void write_visualisation(const Mesh& mesh, const std::size_t cell_dim,
                             const std::string name);

    /// Read vector from file
    void read(GenericVector& x, const std::string dataset_name,
              const bool use_partition_from_file=true);

    /// Read Mesh from file
    void read(Mesh& mesh, const std::string name);

    /// Check if dataset exists in HDF5 file
    bool has_dataset(const std::string dataset_name) const;

    /// Flush buffered I/O to disk
    void flush();

  private:

    // Friend
    friend class XDMFFile;

    // Read a mesh which has locally indexed topology and repartition
    void read_mesh_repartition(Mesh &input_mesh,
                               const std::string coordinates_name,
                               const std::string topology_name);

    // Convert LocalMeshData into a Mesh, when running serially
    void build_local_mesh(Mesh &mesh, const LocalMeshData& mesh_data);

    // Write contiguous data to HDF5 data set. Data is flattened into
    // a 1D array, e.g. [x0, y0, z0, x1, y1, z1] for a vector in 3D
    template <typename T>
    void write_data(const std::string dataset_name,
                    const std::vector<T>& data,
                    const std::vector<std::size_t> global_size);

    // Search dataset names for one beginning with search_term
    static std::string search_list(const std::vector<std::string>& list,
                                   const std::string& search_term);

    void reorder_vertices_by_global_indices(std::vector<double>& vertex_coords,
              std::size_t gdim, const std::vector<std::size_t>& global_indices);

    // HDF5 file descriptor/handle
    bool hdf5_file_open;
    hid_t hdf5_file_id;

    // Parallel mode
    const bool mpi_io;
  };
  //---------------------------------------------------------------------------
  template <typename T>
  void HDF5File::write_data(const std::string dataset_name,
                            const std::vector<T>& data,
                            const std::vector<std::size_t> global_size)
  {
    dolfin_assert(hdf5_file_open);

    //FIXME: Get groups from dataset_name and recursively create groups
    const std::string group_name(dataset_name, 0, dataset_name.rfind('/'));

    // Check that group exists and create if required
    if (!HDF5Interface::has_group(hdf5_file_id, group_name))
      HDF5Interface::add_group(hdf5_file_id, group_name);

    /*
    if (global_size.size() > 2)
    {
      dolfin_error("HDF5File.h",
                   "write data set to HDF5 file",
                   "Writing data of rank > 2 is not yet supported. It will be fixed soon");
    }
    */

    dolfin_assert(global_size.size() > 0);

    // Get number of 'items'
    std::size_t num_local_items = 1;
    for (std::size_t i = 1; i < global_size.size(); ++i)
      num_local_items *= global_size[i];
    num_local_items = data.size()/num_local_items;

    // Compute offet
    const std::size_t offset = MPI::global_offset(num_local_items, true);
    std::pair<std::size_t, std::size_t> range(offset, offset + num_local_items);

    // Write data to HDF5 file
    HDF5Interface::write_dataset(hdf5_file_id, dataset_name, data,
                                 range, global_size, mpi_io, false);
  }
  //---------------------------------------------------------------------------

}

#endif
#endif
