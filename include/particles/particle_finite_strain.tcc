//! Construct a particle with id and coordinates
template <unsigned Tdim>
mpm::ParticleFiniteStrain<Tdim>::ParticleFiniteStrain(Index id,
                                                      const VectorDim& coord)
    : mpm::Particle<Tdim>(id, coord) {
  // Logger
  std::string logger = "particle_finite_strain" + std::to_string(Tdim) +
                       "d::" + std::to_string(id);
  console_ = std::make_unique<spdlog::logger>(logger, mpm::stdout_sink);
}

//! Construct a particle with id, coordinates and status
template <unsigned Tdim>
mpm::ParticleFiniteStrain<Tdim>::ParticleFiniteStrain(Index id,
                                                      const VectorDim& coord,
                                                      bool status)
    : mpm::Particle<Tdim>(id, coord, status) {
  //! Logger
  std::string logger = "particle_finite_strain" + std::to_string(Tdim) +
                       "d::" + std::to_string(id);
  console_ = std::make_unique<spdlog::logger>(logger, mpm::stdout_sink);
}

// Compute stress
template <unsigned Tdim>
void mpm::ParticleFiniteStrain<Tdim>::compute_stress() noexcept {
  // Check if material ptr is valid
  assert(this->material() != nullptr);
  // Calculate stress
  this->stress_ =
      (this->material())
          ->compute_stress(stress_, deformation_gradient_,
                           deformation_gradient_increment_, this,
                           &state_variables_[mpm::ParticlePhase::Solid]);

  // Update deformation gradient
  this->deformation_gradient_ =
      this->deformation_gradient_increment_ * this->deformation_gradient_;
}

// Compute strain of the particle
template <unsigned Tdim>
void mpm::ParticleFiniteStrain<Tdim>::compute_strain(double dt) noexcept {
  // Compute deformation gradient increment
  // Note: Deformation gradient must be updated after compute_stress
  deformation_gradient_increment_ =
      this->compute_deformation_gradient_increment(
          dn_dx_, mpm::ParticlePhase::Solid, dt);

  // Update volume and mass density
  const double deltaJ = this->deformation_gradient_increment_.determinant();
  this->volume_ *= deltaJ;
  this->mass_density_ /= deltaJ;
}

//! Function to reinitialise material to be run at the beginning of each time
template <unsigned Tdim>
void mpm::ParticleFiniteStrain<Tdim>::initialise_constitutive_law() noexcept {
  // Check if material ptr is valid
  assert(this->material() != nullptr);

  // Reset material to be Elastic
  material_[mpm::ParticlePhase::Solid]->initialise(
      &state_variables_[mpm::ParticlePhase::Solid]);

  // Compute initial consititutive matrix
  this->constitutive_matrix_ =
      material_[mpm::ParticlePhase::Solid]->compute_consistent_tangent_matrix(
          stress_, previous_stress_, deformation_gradient_,
          deformation_gradient_increment_, this,
          &state_variables_[mpm::ParticlePhase::Solid]);
}

//! Map mass and material stiffness matrix to cell
//! (used in poisson equation LHS)
template <unsigned Tdim>
inline bool mpm::ParticleFiniteStrain<Tdim>::map_stiffness_matrix_to_cell(
    double newmark_beta, double dt, bool quasi_static) {
  bool status = true;
  try {
    // Check if material ptr is valid
    assert(this->material() != nullptr);

    // Compute material stiffness matrix
    this->map_material_stiffness_matrix_to_cell();

    // Compute mass matrix
    if (!quasi_static) this->map_mass_matrix_to_cell(newmark_beta, dt);

    // Compute geometric stiffness matrix
    this->map_geometric_stiffness_matrix_to_cell();

  } catch (std::exception& exception) {
    console_->error("{} #{}: {}\n", __FILE__, __LINE__, exception.what());
    status = false;
  }
  return status;
}

//! Map geometric stiffness matrix to cell (used in equilibrium equation LHS)
template <unsigned Tdim>
inline bool
    mpm::ParticleFiniteStrain<Tdim>::map_geometric_stiffness_matrix_to_cell() {
  bool status = true;
  try {
    // Calculate G matrix
    const Eigen::MatrixXd gmatrix = this->compute_gmatrix();

    // Stress component matrix
    Eigen::MatrixXd stress_matrix = compute_stress_matrix();

    // Compute local geometric stiffness matrix
    cell_->compute_local_material_stiffness_matrix(gmatrix, stress_matrix,
                                                   volume_);
  } catch (std::exception& exception) {
    console_->error("{} #{}: {}\n", __FILE__, __LINE__, exception.what());
    status = false;
  }
  return status;
}

// Compute G matrix for geometric stiffness
template <>
inline Eigen::MatrixXd
    mpm::ParticleFiniteStrain<1>::compute_gmatrix() noexcept {
  Eigen::MatrixXd gmatrix;
  gmatrix.resize(1, this->nodes_.size());
  gmatrix.setZero();

  for (unsigned i = 0; i < this->nodes_.size(); ++i) {
    gmatrix(0, i) = dn_dx_(i, 0);
  }
  return gmatrix;
}

// Compute G matrix for geometric stiffness
template <>
inline Eigen::MatrixXd
    mpm::ParticleFiniteStrain<2>::compute_gmatrix() noexcept {
  Eigen::MatrixXd gmatrix;
  gmatrix.resize(4, 2 * this->nodes_.size());
  gmatrix.setZero();

  for (unsigned i = 0; i < this->nodes_.size(); ++i) {
    gmatrix(0, 2 * i) = dn_dx_(i, 0);
    gmatrix(1, 2 * i) = dn_dx_(i, 1);
    gmatrix(2, 2 * i + 1) = dn_dx_(i, 0);
    gmatrix(3, 2 * i + 1) = dn_dx_(i, 1);
  }
  return gmatrix;
}

// Compute G matrix for geometric stiffness
template <>
inline Eigen::MatrixXd
    mpm::ParticleFiniteStrain<3>::compute_gmatrix() noexcept {
  Eigen::MatrixXd gmatrix;
  gmatrix.resize(9, 3 * this->nodes_.size());
  gmatrix.setZero();

  for (unsigned i = 0; i < this->nodes_.size(); ++i) {
    gmatrix(0, 3 * i) = dn_dx_(i, 0);
    gmatrix(1, 3 * i) = dn_dx_(i, 1);
    gmatrix(2, 3 * i) = dn_dx_(i, 2);
    gmatrix(3, 3 * i + 1) = dn_dx_(i, 0);
    gmatrix(4, 3 * i + 1) = dn_dx_(i, 1);
    gmatrix(5, 3 * i + 1) = dn_dx_(i, 2);
    gmatrix(6, 3 * i + 2) = dn_dx_(i, 0);
    gmatrix(7, 3 * i + 2) = dn_dx_(i, 1);
    gmatrix(8, 3 * i + 2) = dn_dx_(i, 2);
  }
  return gmatrix;
}

// Compute stress component matrix for geometric stiffness
template <>
inline Eigen::MatrixXd
    mpm::ParticleFiniteStrain<1>::compute_stress_matrix() noexcept {
  Eigen::MatrixXd stress_matrix;
  stress_matrix.resize(1, 1);
  stress_matrix.setZero();

  stress_matrix(0, 0) = this->stress_(0);

  return stress_matrix;
}

// Compute stress component matrix for geometric stiffness
template <>
inline Eigen::MatrixXd
    mpm::ParticleFiniteStrain<2>::compute_stress_matrix() noexcept {
  Eigen::MatrixXd stress_matrix;
  stress_matrix.resize(4, 4);
  stress_matrix.setZero();

  for (unsigned i = 0; i <= 1; i++) {
    stress_matrix(2 * i + 0, 2 * i + 0) = this->stress_(0);
    stress_matrix(2 * i + 0, 2 * i + 1) = this->stress_(3);
    stress_matrix(2 * i + 1, 2 * i + 0) = this->stress_(3);
    stress_matrix(2 * i + 1, 2 * i + 1) = this->stress_(1);
  }

  return stress_matrix;
}

// Compute stress component matrix for geometric stiffness
template <>
inline Eigen::MatrixXd
    mpm::ParticleFiniteStrain<3>::compute_stress_matrix() noexcept {
  Eigen::MatrixXd stress_matrix;
  stress_matrix.resize(9, 9);
  stress_matrix.setZero();

  for (unsigned i = 0; i <= 2; i++) {
    stress_matrix(3 * i + 0, 3 * i + 0) = this->stress_(0);
    stress_matrix(3 * i + 0, 3 * i + 1) = this->stress_(3);
    stress_matrix(3 * i + 0, 3 * i + 2) = this->stress_(5);
    stress_matrix(3 * i + 1, 3 * i + 0) = this->stress_(3);
    stress_matrix(3 * i + 1, 3 * i + 1) = this->stress_(1);
    stress_matrix(3 * i + 1, 3 * i + 2) = this->stress_(4);
    stress_matrix(3 * i + 2, 3 * i + 0) = this->stress_(5);
    stress_matrix(3 * i + 2, 3 * i + 1) = this->stress_(4);
    stress_matrix(3 * i + 2, 3 * i + 2) = this->stress_(2);
  }

  return stress_matrix;
}

// Compute stress using implicit updating scheme
template <unsigned Tdim>
void mpm::ParticleFiniteStrain<Tdim>::compute_stress_newmark() noexcept {
  // Check if material ptr is valid
  assert(this->material() != nullptr);
  // Clone state variables
  auto temp_state_variables = state_variables_[mpm::ParticlePhase::Solid];
  // Calculate stress
  this->stress_ = (this->material())
                      ->compute_stress(previous_stress_, deformation_gradient_,
                                       deformation_gradient_increment_, this,
                                       &temp_state_variables);

  // Compute current consititutive matrix
  this->constitutive_matrix_ =
      material_[mpm::ParticlePhase::Solid]->compute_consistent_tangent_matrix(
          stress_, previous_stress_, deformation_gradient_,
          deformation_gradient_increment_, this, &temp_state_variables);
}

// Compute deformation gradient increment and volume of the particle
template <unsigned Tdim>
void mpm::ParticleFiniteStrain<Tdim>::compute_strain_volume_newmark() noexcept {
  // Compute volume and mass density at the previous time step
  double deltaJ = this->deformation_gradient_increment_.determinant();
  this->volume_ /= deltaJ;
  this->mass_density_ *= deltaJ;

  // Compute deformation gradient increment from previous time step
  this->deformation_gradient_increment_ =
      this->compute_deformation_gradient_increment(this->dn_dx_,
                                                   mpm::ParticlePhase::Solid);

  // Update volume and mass density
  deltaJ = this->deformation_gradient_increment_.determinant();
  this->volume_ *= deltaJ;
  this->mass_density_ /= deltaJ;
}

// Compute Hencky strain
template <unsigned Tdim>
inline Eigen::Matrix<double, 6, 1>
    mpm::ParticleFiniteStrain<Tdim>::compute_hencky_strain() const {

  // Left Cauchy-Green strain
  const Eigen::Matrix<double, 3, 3> left_cauchy_green_tensor =
      deformation_gradient_ * deformation_gradient_.transpose();

  // Principal values of left Cauchy-Green strain
  Eigen::Matrix<double, 3, 3> directors = Eigen::Matrix<double, 3, 3>::Zero();
  const Eigen::Matrix<double, 3, 1> principal_left_cauchy_green_strain =
      mpm::materials::principal_tensor(left_cauchy_green_tensor, directors);

  // Principal value of Hencky (logarithmic) strain
  Eigen::Matrix<double, 3, 3> principal_hencky_strain =
      Eigen::Matrix<double, 3, 3>::Zero();
  principal_hencky_strain.diagonal() =
      0.5 * principal_left_cauchy_green_strain.array().log();

  // Hencky strain tensor and vector
  const Eigen::Matrix<double, 3, 3> hencky_strain =
      directors * principal_hencky_strain * directors.transpose();
  Eigen::Matrix<double, 6, 1> hencky_strain_vector;
  hencky_strain_vector(0) = hencky_strain(0, 0);
  hencky_strain_vector(1) = hencky_strain(1, 1);
  hencky_strain_vector(2) = hencky_strain(2, 2);
  hencky_strain_vector(3) = 2. * hencky_strain(0, 1);
  hencky_strain_vector(4) = 2. * hencky_strain(1, 2);
  hencky_strain_vector(5) = 2. * hencky_strain(2, 0);

  return hencky_strain_vector;
}

// Update stress and strain after convergence of Newton-Raphson iteration
template <unsigned Tdim>
void mpm::ParticleFiniteStrain<Tdim>::update_stress_strain() noexcept {
  // Update converged stress
  this->stress_ =
      (this->material())
          ->compute_stress(this->previous_stress_, this->deformation_gradient_,
                           this->deformation_gradient_increment_, this,
                           &state_variables_[mpm::ParticlePhase::Solid]);

  // Update initial stress of the time step
  this->previous_stress_ = this->stress_;

  // Update deformation gradient
  this->deformation_gradient_ =
      this->deformation_gradient_increment_ * this->deformation_gradient_;

  // Volumetric strain increment
  this->dvolumetric_strain_ =
      (this->deformation_gradient_increment_.determinant() - 1.0);

  // Update volumetric strain at particle position (not at centroid)
  this->volumetric_strain_centroid_ += this->dvolumetric_strain_;

  // Reset deformation gradient increment
  this->deformation_gradient_increment_.setIdentity();
}
