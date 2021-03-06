/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using std::string;
using std::vector;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1.
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method
   *   (and others in this file).
   */
  num_particles = 100;  // TODO: Set the number of particles
  // This line creates a normal (Gaussian) distribution for x
  std::default_random_engine gen;
  std::normal_distribution<double> dist_x(x, std[0]);
  std::normal_distribution<double> dist_y(y, std[1]);
  std::normal_distribution<double> dist_theta(theta, std[2]);
  double particle_x = dist_x(gen);
  double particle_y = dist_y(gen);
  double particle_theta = dist_theta(gen);
  for (unsigned int ii=0; ii<num_particles; ++ii)
  {
    Particle p;
    p.x = particle_x;
    p.y = particle_y;
    p.theta = particle_theta;
    p.weight = 1.;
    this->particles.push_back(p);
    this->weights.push_back(1.);

  }
  this->is_initialized = true;

}

void ParticleFilter::prediction(double delta_t, double std_pos[],
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
   std::default_random_engine gen;

   for (unsigned int ii=0; ii<num_particles; ++ii)
   {
       Particle p = this->particles[ii];
       double particle_x=p.x, particle_y=p.y, particle_theta=p.theta;
       if (fabs(yaw_rate)<0.0000001)
       {
            particle_x += velocity*delta_t*cos(p.theta);
            particle_y += velocity*delta_t*sin(p.theta);
       }
       else
       {
            particle_x += velocity/yaw_rate*(sin(p.theta+yaw_rate*delta_t) - sin(p.theta));
            particle_y += velocity/yaw_rate*(cos(p.theta) - cos(p.theta+yaw_rate*delta_t));
            particle_theta += yaw_rate*delta_t;
       }
       std::normal_distribution<double> dist_x(particle_x, std_pos[0]);
       std::normal_distribution<double> dist_y(particle_y, std_pos[1]);
       std::normal_distribution<double> dist_theta(particle_theta, std_pos[2]);
       this->particles[ii].x = dist_x(gen);
       this->particles[ii].y = dist_y(gen);
       this->particles[ii].theta = dist_theta(gen);
    }

}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted,
                                     vector<LandmarkObs>& observations,
                                     Particle &particle) {
  /**
   * TODO: Find the predicted measurement that is closest to each
   *   observed measurement and assign the observed measurement to this
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will
   *   probably find it useful to implement this method and use it as a helper
   *   during the updateWeights phase.
   */
   vector<int> associations;
  for (std::size_t ii=0; ii<observations.size(); ++ii)
  {
    double temp_dist, min_dist = std::numeric_limits<double>::max();
    for (std::size_t jj=0; jj<predicted.size(); ++jj)
    {
      temp_dist = dist(predicted[jj].x, predicted[jj].y, observations[ii].x, observations[ii].y);
      if (temp_dist<min_dist)
      {
          min_dist = temp_dist;
          observations[ii].id = jj;
          associations.push_back(predicted.at(jj).id);

      }
    }
  }
  //particle = SetAssociations(particle, associations, )

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[],
                                   const vector<LandmarkObs> &observations,
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a multi-variate Gaussian
   *   distribution. You can read more about this distribution here:
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system.
   *   Your particles are located according to the MAP'S coordinate system.
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */
   for(std::size_t ii=0; ii<this->particles.size(); ++ii)
   {
       // get predicted landmarks: only those within sensor_range
       vector<LandmarkObs> predicted;
       for (std::size_t kk=0; kk<map_landmarks.landmark_list.size(); ++kk)
       {
           if (dist(particles.at(ii).x, particles.at(ii).y,
                map_landmarks.landmark_list.at(kk).x_f, map_landmarks.landmark_list.at(kk).y_f)
                < sensor_range)
              {
                  LandmarkObs temp;
                  temp.x = map_landmarks.landmark_list.at(kk).x_f;
                  temp.y = map_landmarks.landmark_list.at(kk).y_f;
                  temp.id = kk; //let's use the landmark index
                  predicted.push_back(temp);
              }
       }
       vector<LandmarkObs> temp_obs = observations;
       for (std::size_t jj=0; jj<observations.size(); ++jj)
       {
         //Convert observations from car to map coordinates
         double cos_t = cos(particles[ii].theta);
         double sin_t = sin(particles[ii].theta);
         temp_obs[jj].x = particles[ii].x + cos_t*observations[jj].x
                - sin_t*observations[jj].y;
         temp_obs[jj].y = particles[ii].y + sin_t*observations[jj].x
               + cos_t*observations[jj].y;
       }
       this->dataAssociation(predicted, temp_obs, particles[ii]);
       this->particles[ii].weight = 1.;
       for (std::size_t jj=0; jj<temp_obs.size(); ++jj)
       {
           double pr_x, pr_y;
           /*for (unsigned int ll = 0; ll < predicted.size(); ll++)
           {
              if (predicted[ll].id == temp_obs[jj].id)
              {
                pr_x = predicted[ll].x;
                pr_y = predicted[ll].y;
                break;
              }
          }*/
          pr_x = predicted[temp_obs.at(jj).id].x;
          pr_y = predicted[temp_obs.at(jj).id].y;
          double val = multiv_prob(std_landmark[0], std_landmark[1],
               temp_obs.at(jj).x, temp_obs.at(jj).y,
               pr_x,
               pr_y);
           this->particles[ii].weight *= val;
       }
       this->weights[ii] = this->particles[ii].weight;

   }
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional
   *   to their weight.
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
   vector<double> new_weights;
   std::default_random_engine generator;
   std::discrete_distribution<> distribution(this->weights.begin(), this->weights.end());

   std::vector<Particle> new_particles;

   for (size_t i=0; i<this->particles.size(); ++i) {
     new_particles.push_back(this->particles.at(distribution(generator)));
     }
   this->particles = new_particles;

}

void ParticleFilter::SetAssociations(Particle& particle,
                                     const vector<int>& associations,
                                     const vector<double>& sense_x,
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association,
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}
