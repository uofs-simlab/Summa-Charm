
#ifndef TOLERANCE_SETTINGS_HPP
#define TOLERANCE_SETTINGS_HPP

#define SUCCESS 0
#define FAILURE -1
#define MISSING_INT -9999
#define MISSING_DOUBLE -9999.0
#define OUTPUT_TIMESTEPS 500
#define NUM_PARTITIONS 8
#define OUTPUT_FREQUENCY 1000
#define GRU_PER_JOB 1000


class ToleranceSettings
{
public:
    int be_steps_;

    double rel_tol_temp_cas_ ;
    double rel_tol_temp_veg_ ;
    double rel_tol_wat_veg_ ;
    double rel_tol_temp_soil_snow_ ;
    double rel_tol_wat_snow_ ;
    double rel_tol_matric_ ;
    double rel_tol_aquifr_ ;
    
    double abs_tol_temp_cas_ ;
    double abs_tol_temp_veg_;
    double abs_tol_wat_veg_;
    double abs_tol_temp_soil_snow_;
    double abs_tol_wat_snow_;
    double abs_tol_matric_ ;
    double abs_tol_aquifr_ ;
 
    ToleranceSettings(
        int be_steps = MISSING_INT,
        double rel_tol_temp_cas = 0.0,
        double rel_tol_temp_veg = 0.0, 
        double rel_tol_wat_veg = 0.0,
        double rel_tol_temp_soil_snow = 0.0, 
        double rel_tol_wat_snow = 0.0,
        double rel_tol_matric = 0.0, 
        double rel_tol_aquifr = 0.0,
        double abs_tol_temp_cas = 0.0, 
        double abs_tol_temp_veg = 0.0,
        double abs_tol_wat_veg = 0.0, 
        double abs_tol_temp_soil_snow = 0.0,
        double abs_tol_wat_snow = 0.0, 
        double abs_tol_matric = 0.0,
        double abs_tol_aquifr = 0.0) :
          be_steps_(be_steps),
          rel_tol_temp_cas_(rel_tol_temp_cas),
          rel_tol_temp_veg_(rel_tol_temp_veg), 
          rel_tol_wat_veg_(rel_tol_wat_veg),
          rel_tol_temp_soil_snow_(rel_tol_temp_soil_snow), 
          rel_tol_wat_snow_(rel_tol_wat_snow), 
          rel_tol_matric_(rel_tol_matric),
          rel_tol_aquifr_(rel_tol_aquifr), 
          abs_tol_temp_cas_(abs_tol_temp_cas),
          abs_tol_temp_veg_(abs_tol_temp_veg), 
          abs_tol_wat_veg_(abs_tol_wat_veg), 
          abs_tol_temp_soil_snow_(abs_tol_temp_soil_snow),
          abs_tol_wat_snow_(abs_tol_wat_snow), 
          abs_tol_matric_(abs_tol_matric),
          abs_tol_aquifr_(abs_tol_aquifr) {};

    ~ToleranceSettings() {};

    std::string toString() {
        std::string str = "Tolerance Settings:\n";
      str += "BE Steps: " + std::to_string(be_steps_) + "\n";
      str += "Rel Tol Temp Veg: " + std::to_string(rel_tol_temp_veg_) + "\n";
      str += "Rel Tol Temp Cas: " + std::to_string(rel_tol_temp_cas_) + "\n";
      str += "Rel Tol Wat Veg: " + std::to_string(rel_tol_wat_veg_) + "\n";
      str += "Rel Tol Temp Soil Snow: " + std::to_string(rel_tol_temp_soil_snow_) + "\n";
      str += "Rel Tol Wat Snow: " + std::to_string(rel_tol_wat_snow_) + "\n";
      str += "Rel Tol Matric: " + std::to_string(rel_tol_matric_) + "\n";
      str += "Rel Tol Aquifr: " + std::to_string(rel_tol_aquifr_) + "\n";
      str += "Abs Tol Temp Cas: " + std::to_string(abs_tol_temp_cas_) + "\n";
      str += "Abs Tol Temp Veg: " + std::to_string(abs_tol_temp_veg_) + "\n";
      str += "Abs Tol Wat Veg: " + std::to_string(abs_tol_wat_veg_) + "\n";
      str += "Abs Tol Temp Soil Snow: " + std::to_string(abs_tol_temp_soil_snow_) + "\n";
      str += "Abs Tol Wat Snow: " + std::to_string(abs_tol_wat_snow_) + "\n";
      str += "Abs Tol Matric: " + std::to_string(abs_tol_matric_) + "\n";
      str += "Abs Tol Aquifr: " + std::to_string(abs_tol_aquifr_) + "\n";
      return str;
    }

    // PUP method for Charm++ serialization
    template <typename PUPER>
    void pup(PUPER &p) {
        p | be_steps_;
        p | rel_tol_temp_cas_;
        p | rel_tol_temp_veg_;
        p | rel_tol_wat_veg_;
        p | rel_tol_temp_soil_snow_;
        p | rel_tol_wat_snow_;
        p | rel_tol_matric_;
        p | rel_tol_aquifr_;
        p | abs_tol_temp_cas_;
        p | abs_tol_temp_veg_;
        p | abs_tol_wat_veg_;
        p | abs_tol_temp_soil_snow_;
        p | abs_tol_wat_snow_;
        p | abs_tol_matric_;
        p | abs_tol_aquifr_;
    }
};

#endif // TOLERANCE_SETTINGS_HPP