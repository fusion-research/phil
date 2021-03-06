#include <phil/localization/particle_filter.h>

#include <bfl/model/linearanalyticsystemmodel_gaussianuncertainty.h>
#include <bfl/filter/bootstrapfilter.h>

namespace phil {

constexpr unsigned int ParticleFilter::NUM_SAMPLES;

ParticleFilter::ParticleFilter()
    : system_model(nullptr), yaw_measurement_model(nullptr), acc_measurement_model(nullptr) {
  MatrixWrapper::ColumnVector prior_mean(localization::N);
  prior_mean << 0, 0, 0, 0, 0, 0, 0, 0, 0;
  MatrixWrapper::SymmetricMatrix prior_covariance(localization::N);
  prior_covariance = 0;
  prior_covariance(1, 1) = 0.001;
  prior_covariance(2, 2) = 0.001;
  prior_covariance(3, 3) = 0.001;
  prior_covariance(4, 4) = 0.001;
  prior_covariance(5, 5) = 0.001;
  prior_covariance(6, 6) = 0.001;
  prior_covariance(7, 7) = 0.001;
  prior_covariance(8, 8) = 0.001;
  prior_covariance(9, 9) = 0.001;
  BFL::Gaussian prior(prior_mean, prior_covariance);

  std::vector<BFL::Sample<MatrixWrapper::ColumnVector> > prior_samples(NUM_SAMPLES);
  prior.SampleFrom(prior_samples, NUM_SAMPLES, CHOLESKY, nullptr);
  // With these samples we create a discrete prior distribution, a monte carlo pdf:
  prior_discrete = std::make_unique<BFL::MCPdf<BFL::ColumnVector> >(NUM_SAMPLES, 3);
  prior_discrete->ListOfSamplesSet(prior_samples);
  filter = std::make_unique<PF>(prior_discrete.get(), 0, NUM_SAMPLES / 4.0);

  // Define x = f(x,u)
  MatrixWrapper::ColumnVector system_noise_mean(localization::N);
  system_noise_mean = 0.0;
  MatrixWrapper::SymmetricMatrix system_noise_covariance(localization::N);
  system_noise_covariance = 0.0;
  system_noise_covariance(1, 1) = 0.001;
  system_noise_covariance(2, 2) = 0.001;
  system_noise_covariance(3, 3) = 0.001;
  system_noise_covariance(4, 4) = 0.001;
  system_noise_covariance(5, 5) = 0.001;
  system_noise_covariance(6, 6) = 0.001;
  system_noise_covariance(7, 7) = 0.001;
  system_noise_covariance(8, 8) = 0.001;
  system_noise_covariance(9, 9) = 0.001;
  BFL::Gaussian system_uncertainty(system_noise_mean, system_noise_covariance);
  system_pdf = std::make_unique<localization::EncoderControlModel>(system_uncertainty);
  system_model = std::make_unique<BFL::AnalyticSystemModelGaussianUncertainty>(system_pdf.get());

  // Construct measurement models for each of our sensor packages
  // First for the yaw measurement which comes from the NavX on the RoboRIO
  constexpr int yaw_dim = 1;
  MatrixWrapper::Matrix yaw_measurement_H(yaw_dim, localization::N);
  yaw_measurement_H << 0, 0, 1, 0, 0, 0, 0, 0, 0;
  MatrixWrapper::ColumnVector yaw_measurement_mean(yaw_dim);
  yaw_measurement_mean = 0;
  MatrixWrapper::SymmetricMatrix yaw_measurement_covariance(yaw_dim);
  yaw_measurement_covariance = 5.163132E-07; // derived by Scott Libert of Kauai Labs
  BFL::Gaussian yaw_measurement_uncertainty(yaw_measurement_mean, yaw_measurement_covariance);
  yaw_measurement_pdf =
      std::make_unique<BFL::LinearAnalyticConditionalGaussian>(yaw_measurement_H, yaw_measurement_uncertainty);
  yaw_measurement_model =
      std::make_unique<BFL::LinearAnalyticMeasurementModelGaussianUncertainty>(yaw_measurement_pdf.get());

  // Second for the world-frame accelerometer measurements which comes from the NavX on the RoboRIO
  constexpr int acc_dim = 2;
  MatrixWrapper::Matrix acc_measurement_H(acc_dim, localization::N);
  acc_measurement_H << 0, 0, 0, 0, 0, 0, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 1, 0;
  MatrixWrapper::ColumnVector acc_measurement_mean(acc_dim);
  acc_measurement_mean = 0;
  MatrixWrapper::SymmetricMatrix acc_measurement_covariance(acc_dim);
  acc_measurement_covariance = 0;
  acc_measurement_covariance(1, 1) = 0.000000001;
  acc_measurement_covariance(2, 2) = 0.000000001;
  BFL::Gaussian acc_measurement_uncertainty(acc_measurement_mean, acc_measurement_covariance);
  acc_measurement_pdf =
      std::make_unique<BFL::LinearAnalyticConditionalGaussian>(acc_measurement_H, acc_measurement_uncertainty);
  acc_measurement_model =
      std::make_unique<BFL::LinearAnalyticMeasurementModelGaussianUncertainty>(acc_measurement_pdf.get());

  constexpr int camera_dim = 3;
  MatrixWrapper::Matrix camera_measurement_H(camera_dim, localization::N);
  camera_measurement_H << 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 0, 0, 0, 0, 0, 0;
  MatrixWrapper::ColumnVector camera_measurement_mean(camera_dim);
  camera_measurement_mean = 0;
  MatrixWrapper::SymmetricMatrix camera_measurement_covariance(camera_dim);
  camera_measurement_covariance = 0;
  camera_measurement_covariance(1, 1) = 0.0001;
  camera_measurement_covariance(2, 2) = 0.0001;
  camera_measurement_covariance(3, 3) = 0.0001;
  BFL::Gaussian camera_measurement_uncertainty(camera_measurement_mean, camera_measurement_covariance);
  camera_measurement_pdf =
      std::make_unique<BFL::LinearAnalyticConditionalGaussian>(camera_measurement_H, camera_measurement_uncertainty);
  camera_measurement_model =
      std::make_unique<BFL::LinearAnalyticMeasurementModelGaussianUncertainty>(camera_measurement_pdf.get());

  constexpr int beacon_dim = 2;
  MatrixWrapper::Matrix beacon_measurement_H(beacon_dim, localization::N);
  beacon_measurement_H << 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 0, 0, 0, 0, 0, 0, 0;
  MatrixWrapper::ColumnVector beacon_measurement_mean(beacon_dim);
  beacon_measurement_mean = 0;
  MatrixWrapper::SymmetricMatrix beacon_measurement_covariance(beacon_dim);
  beacon_measurement_covariance = 0;
  beacon_measurement_covariance(1, 1) = 0.0001;
  beacon_measurement_covariance(2, 2) = 0.0001;
  BFL::Gaussian beacon_measurement_uncertainty(beacon_measurement_mean, beacon_measurement_covariance);
  beacon_measurement_pdf =
      std::make_unique<BFL::LinearAnalyticConditionalGaussian>(beacon_measurement_H, beacon_measurement_uncertainty);
  beacon_measurement_model =
      std::make_unique<BFL::LinearAnalyticMeasurementModelGaussianUncertainty>(beacon_measurement_pdf.get());
}

void ParticleFilter::ZeroVelocityUpdate() {
  for (unsigned int i = 0; i < NUM_SAMPLES; ++i) {
    auto &samples = filter->PostGet()->ListOfSamplesGet();
    auto sample = samples.at(i);
    auto state = sample.ValueGet();
    state(4) = 0;
    state(5) = 0;
    state(6) = 0;
    sample.ValueSet(state);
  }
}

}
