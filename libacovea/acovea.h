//---------------------------------------------------------------------
//  ACOVEA -- Analysis of Compiler Options Via Evolution Algorithm
//
//  acovea.h
//
//  Class definitions for the ACOVEA algorithm
//---------------------------------------------------------------------
//
//  Copyright 2003, 2004, 2005 Scott Robert Ladd
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the
//      Free Software Foundation, Inc.
//      59 Temple Place - Suite 330
//      Boston, MA 02111-1307, USA.
//
//-----------------------------------------------------------------------
//
//  For more information on this software package, please visit
//  Scott's web site, Coyote Gulch Productions, at:
//
//      http://www.coyotegulch.com
//  
//-----------------------------------------------------------------------

#if !defined(ACOVEA_H)
#define ACOVEA_H

#include "libevocosm/evocommon.h"
#include "libevocosm/evocosm.h"
#include "libevocosm/roulette.h"

namespace acovea
{
    using namespace libevocosm;
    using namespace std;
    
    //----------------------------------------------------------
    // Fitness scaling type
    enum optimization_mode
    {
        OPTIMIZE_SPEED,
        OPTIMIZE_SIZE,
        OPTIMIZE_RETVAL,
    };

    //----------------------------------------------------------
    // objects global to several classes
    class common : protected libevocosm::globals
    {
        // class exists to provide a singular path to libevocosm::globals
    };
    
    //----------------------------------------------------------
    // settings tracker
    //      an option with settings returns an object of this type containing
    //      option-specific tracking data (value, for example); several trackers
    //      can be accumulated to get cross-population totals
    class settings_tracker
    {
        public:
            // get a string representing this settings tracker
            virtual string get_settings_text()
            {
                return string("none");
            }

            // accumulate information        
            virtual settings_tracker & operator += (const settings_tracker & tracker)
            {
                return *this;
            }
        
            // virtual destructor (does nothing unless overridden)
            virtual ~settings_tracker()
            {
                // nada
            }
    };
    
    //----------------------------------------------------------
    // abstract definition of a application option or switch
    class option : public common
    {
        public:
            // creation constructor
            option(bool a_enabled = true);
        
            // copy constructor
            option(const option & a_source);
            
            // assignment 
            option & operator = (const option & a_source);
            
            // clone
            virtual option * clone() = 0;
            
            // properties
            bool is_enabled() const
            {
                return m_enabled;
            }
            
            void set_enabled(bool a_enabled)
            {
                m_enabled = a_enabled;
            }
            
            // averaging
            virtual bool has_settings()
            {
                return false;
            }
            
            virtual settings_tracker * alloc_settings_tracker()
            {
                return new settings_tracker();
            }
            
            // get the string for this option
            virtual string get() const = 0;
        
            // get the unadorned name of this option
            virtual vector<string> get_choices() const = 0;
        
            // get the setting for this option
            virtual int get_setting() const = 0;
        
            // randomize settings of this option
            virtual void randomize();

            // mutate this option
            virtual void mutate();
        
            // virtual destructor (does nothing unless overridden)
            virtual ~option() = 0;
            
        protected:
            // is this option enabled (included in command line)
            bool m_enabled;
    };
            
    //----------------------------------------------------------
    // an option that is just a string
    class simple_option : public option
    {
        public:
            // creation constructor
            simple_option(const string & a_name, bool a_enabled = true);
        
            // creation constructor
            simple_option(const char * a_name, bool a_enabled = true);
        
            // copy constructor
            simple_option(const simple_option & a_source);
            
            // assignment operator
            simple_option & operator = (const simple_option & a_source);
        
            // clone
            virtual option * clone();
            
            // get the string for this option
            virtual string get() const
            {
                return m_name;
            }
        
            // get the unadorned name of this option
            virtual vector<string> get_choices() const
            {
                vector<string> result;
                result.push_back(m_name);
                return result;
            }
            
            // get the setting for this option
            virtual int get_setting() const
            {
                return 0;
            }
        
        protected:
            // name of this option
            string m_name;
    };
            
    //----------------------------------------------------------
    // value settings tracker 
    class tuning_option;
    
    class tuning_settings_tracker : public settings_tracker
    {
        public:
            // constructor
            tuning_settings_tracker(const tuning_option & option);
                
            // copy constructor
            tuning_settings_tracker(const tuning_settings_tracker & source);
                
            // assignment
            tuning_settings_tracker & operator = (const tuning_settings_tracker & source);
                
            // get a string representing this settings tracker
            virtual string get_settings_text();

            // accumulate information        
            virtual settings_tracker & operator += (const settings_tracker & tracker);
        
            // virtual destructor (does nothing unless overridden)
            virtual ~tuning_settings_tracker()
            {
                // nada
            }
        
        private:
            // list of values
            vector<int> m_values;
        
            // sum of enabled options
            int m_sum;
        
            // count of options enabled
            int m_count;
    };
            
    //----------------------------------------------------------
    // an option or switch that requires an integer argument
    class tuning_option : public simple_option
    {
        public:
            // creation constructor
            tuning_option(const string & a_name,
                          bool   a_enabled,
                          int a_default,
                          int a_min_value,
                          int a_max_value,
                          int a_step,
                          char   a_separator = '=');

            // copy constructor
            tuning_option(const tuning_option & a_source);

            // assignment operator
            tuning_option & operator = (const tuning_option & a_source);

            // clone
            virtual option * clone();
            
            // get the string for this option
            virtual string get() const;
        
            // get the value of this option
            int get_value() const
            {
                return m_value;
            }

            // get the value of this option
            int set_value(int a_value) 
            {
                m_value = a_value;
                
                if (m_value < m_min_value)
                    m_value = m_min_value;
                
                if (m_value > m_max_value)
                    m_value = m_max_value;
            }

            // get the value of this option
            int get_default() const
            {
                return m_default;
            }

            // averaging
            virtual bool has_settings()
            {
                return true;
            }
            
            virtual settings_tracker * alloc_settings_tracker()
            {
                return new tuning_settings_tracker(*this);
            }
            
            // mutate this option
            virtual void mutate();
        
            // virtual destructor (does nothing unless overridden)
            virtual ~tuning_option() { };
            
        protected:
            // current value
            int m_value;
        
            // range of value
            int m_default;
            int m_min_value;
            int m_max_value;
        
            // maximum change in value per mutation
            int m_step;
        
            // separator between name and value
            char m_separator;
    };
    
    //----------------------------------------------------------
    // enum option and settings tracker 
    class enum_option : public option
    {
        public:
            // creation constructor (vector of strings)
            enum_option(const vector<string> & a_choices, bool a_enabled = true);

            // creation constructor (char * array)
            enum_option(const char ** a_choices, size_t a_num_choices, bool a_enabled = true);
            
            // creation constructor (delimited string)
            enum_option(const char * a_choices, bool a_enabled = true);

            // copy constructor
            enum_option(const enum_option & a_source);

            // assignment 
            enum_option & operator = (const enum_option & a_source);

            // clone
            virtual option * clone();

            // get the string for this option
            virtual string get() const;

            // get the unadorned name of this option
            virtual vector<string> get_choices() const
            {
                return m_choices;
            }

            // get current setting
            virtual int get_setting() const
            {
                return m_setting;
            }

            // randomize settings of this option
            virtual void randomize();

            // mutate this option
            virtual void mutate();

            // virtual destructor (does nothing unless overridden)
            virtual ~enum_option();

        protected:
            // which setting?
            int m_setting;

            vector<string> m_choices;
    };

    //----------------------------------------------------------
    // a vector of options, defined as an object so that the
    //   pointers will be automatically deallocated upon
    //   object deletion
    class chromosome : private vector<option *>
    {
        public:
            // constructor
            chromosome();
        
            // copy constructor
            chromosome(const chromosome & a_source);
            
            // assignment
            chromosome & operator = (const chromosome & a_source);
            
            // destructor
            ~chromosome();
            
            // add a new option
            void push_back(option * a_gene)
            {
                vector<option *>::push_back(a_gene);
            }
            
            // get the size of the vector
            size_t size() const
            {
                return vector<option *>::size();
            }
            
            // get a gene by index
            option * operator [] (size_t a_index) const
            {
                if ((vector<option *>::size() > 0) && (a_index < vector<option *>::size()))
                    return vector<option *>::operator [] (a_index);
                else
                    return NULL;
            }
    };

    //----------------------------------------------------------
    // an application configuration
    typedef struct command_elements
    {
        string m_description;
        string m_command;
        string m_flags;
    };

    class application : public common
    {
        public:
            // constructor
            application(const string & config_name);
                
            // copy constructor
            application(const application & a_source);

            // assignment
            application & operator = (const application & a_source);
            
            // get application configuration in XML
            void get_xml(ostream & a_stream) const;
            
            // import an XML element
            void import_element(const char * element, const char ** attr);

            // return a string description of the application and the configuration
            string get_description() const;
            
            // set the description string
            void set_description(const string & a_description);
            
            // return a version string 
            string get_config_version() const;
            
            // set the description string
            void set_config_version(const string & a_version);
            
            // return the name of the configuration file used
            string get_config_name() const;
            
            // set configuration name
            void set_config_name(const string & a_config_name);
            
            // return the command for invoking the application
            command_elements get_prime() const;
            
            // return the flags elements
            vector<command_elements> get_baselines() const;
            
            // get option list
            chromosome get_options() const;
            
            // set option list
            void set_options(const chromosome & a_options);
            
            // get application name
            string get_app_name() const;
            
            // get the get version command
            vector<string> get_get_app_version() const;
        
            // return an execv-compatible argument list for compiling a given program
            vector<string> get_prime_command(const string &     a_input_name,
                                             const string &     a_output_name,
                                             const chromosome & a_options) const;
            
            // return an execv-compatible argument list for compiling a given program
            vector<string> get_command(const command_elements & a_elements,
                                       const string &     a_input_name,
                                       const string &     a_output_name,
                                       const chromosome & a_options) const;
            
            // get a random set of options for this application
            chromosome get_random_options() const;
            
            // breed a new options set from two parents
            chromosome breed(const chromosome & a_parent1,
                             const chromosome & a_parent2) const;
            
            // mutate an option set
            void mutate(chromosome & a_options, double a_mutation_chance) const;
            
            // get the option set size (should be fized for all chromosomes created
            //   by this application)
            size_t chromosome_length() const;
            
            // destructor (to support derived classes)
            ~application();
            
        private:
            string           m_config_name;       // name of the XML configuration file
            string           m_get_app_version;   // command to get version info for application
            string           m_description;       // brief description of this application
            string           m_config_version;    // version of this config
            command_elements m_prime;             // command used to execute the application
            vector<command_elements> m_baselines; // baselines for comparison with evolved solution
            chromosome       m_options;	          // the base list of options/flags
            bool             m_quoted_options;    // should options be handled in quotes?
    };
    
    //----------------------------------------------------------
    // the organism undergoing evolution
    class acovea_organism : public organism<chromosome>, private common
    {
        public:
            //constructor
            acovea_organism();

            //constructor
            acovea_organism(const application & a_target);

            // valid constructor to set explicit genes
            acovea_organism(const application & a_target,
                            const chromosome & a_genes);

            // copy constructor
            acovea_organism(const acovea_organism & a_source);

            // crossover constructor
            acovea_organism(const acovea_organism & a_parent1,
                            const acovea_organism & a_parent2,
                            const application & target);

            // destructor
            virtual ~acovea_organism();

            // assignment
            acovea_organism & operator = (const acovea_organism & a_source);

            // comparison for sorting
            virtual bool operator < (const organism<chromosome> & a_right) const
            {
                return (m_fitness < a_right.fitness());
            }
    };

    //----------------------------------------------------------
    // mutation operator
    class acovea_mutator : public mutator< acovea_organism >, protected common
    {
        public:
            // creation constructor
            acovea_mutator(double a_mutation_rate, const application & a_target);

            // copy constructor
            acovea_mutator(const acovea_mutator & a_source);

            // assignment
            acovea_mutator & operator = (const acovea_mutator & a_source);

            // destructor
            virtual ~acovea_mutator();

            // mutation operation
            virtual void mutate(vector< acovea_organism > & a_population);

            // interrogator
            double mutation_rate() const
            {
                return m_mutation_rate;
            }

        private:
            // probability that a mutation will take place (while rand < rate, mutate)
            double m_mutation_rate;

            // application object that is target of this test
            const application & m_target;
    };

    //----------------------------------------------------------
    // reproduction operator
    class acovea_reproducer : public reproducer< acovea_organism >
    {
        public:
            // creation constructor
            acovea_reproducer(double a_crossover_rate, const application & a_target);

            // copy constructor
            acovea_reproducer(const acovea_reproducer & a_source);

            // destructor
            virtual ~acovea_reproducer();

            // assignment
            acovea_reproducer & operator = (const acovea_reproducer & a_source);

            // interrogator
            double crossover_rate() const
            {
                return m_crossover_rate;
            }

            // reproduction operation
            virtual vector< acovea_organism > breed(const vector< acovea_organism > & a_population,
                                                    size_t a_limit);

        private:
            // probablity of crossover occuring during reporduction
            double m_crossover_rate;

            // application object that is target of this test
            const application & m_target;
    };

    //----------------------------------------------------------
    // an object that watches acovea world events
    typedef struct test_result
    {
        string  m_description;
        string  m_detail;
        double  m_fitness;
        bool    m_acovea_generated;
    };
    
    typedef struct option_zscore
    {
        string  m_name;
        double  m_zscore;
    };
    
    class acovea_listener : public libevocosm::listener
    {
        public:
            // send configuration
            virtual void report_config(const string & a_text) = 0;
        
            // send generation report (every generation)
            virtual void report_generation(size_t a_gen_no, double a_avg_fitness) = 0;
        
            // send final report
            virtual void report_final(vector<test_result> & a_results, vector<option_zscore> & a_zscores) = 0;
    };
    
    class acovea_listener_stdout : public acovea_listener
    {
        public:
            // ping that a generation begins
            virtual void ping_generation_begin(size_t a_generation_number);
        
            // ping that a generation begins
            virtual void ping_generation_end(size_t a_generation_number);
        
            // ping that a population begins
            virtual void ping_population_begin(size_t a_population_number);
            
            // ping that a population begins
            virtual void ping_population_end(size_t a_population_number);
            
            // ping that a fitness test begins
            virtual void ping_fitness_test_begin(size_t a_organism_number);
            
            // ping that a fitness test begins
            virtual void ping_fitness_test_end(size_t a_organism_number);
            
            // send text
            virtual void report(const string & a_text);
            
            // send error message
            virtual void report_error(const string & a_text);
            
            // acovea is finished
            virtual void run_complete();
            
            // yield
            virtual void yield();
            
            // send configuration
            virtual void report_config(const string & a_text);
            
            // send generation report (every generation)
            virtual void report_generation(size_t a_gen_no, double a_avg_fitness);
            
            // send final report
            virtual void report_final(vector<test_result> & a_results, vector<option_zscore> & a_zscores);
    };

    //----------------------------------------------------------
    // fitness landscape
    class acovea_landscape : public landscape< acovea_organism >,
                             protected common
    {
        public:
            // creation constructor
            acovea_landscape(string              a_bench_name,
                             optimization_mode   a_mode,
                             const application & a_target,
                             acovea_listener &   a_listener);

            // copy constructor
            acovea_landscape(const acovea_landscape & a_source);

            // destructor
            ~acovea_landscape();

            // assignment
            acovea_landscape & operator = (const acovea_landscape & a_source);

            // test a single option list
            virtual double test(acovea_organism & a_org, bool a_verbose = false) const;

            // test a population of option lists
            virtual double test(vector< acovea_organism > & a_population) const;

        private:
            // name of application for information display
            string m_input_name;
        
            // testing mode
            optimization_mode m_mode;

            // application object that is target of this test
            const application & m_target;
    };
    
    //----------------------------------------------------------
    // status and statistics
    class acovea_reporter : public reporter<acovea_organism,acovea_landscape>,
                            protected common
    {
        public:
            // creation constructor
            acovea_reporter(string              a_bench_name,
                            size_t              a_number_of_populations,
                            const application & a_target,
                            acovea_listener &   a_listener,
                            optimization_mode   a_mode);

            // destructor
            ~acovea_reporter();
            
            // set config heading
            void set_config_text(const string & a_config_text)
            {
                m_config_text = a_config_text;
            }

            // report status and statistics
            virtual bool report(const vector< vector< acovea_organism > > & a_populations,
                                size_t   a_iteration,
                                double & a_fitness,
                                bool     a_finished = false);

        private:
            // constants for display and loops
            const size_t m_number_of_populations;
            const string m_input_name;
            string m_config_text;
            
            // the threshold for reporting an option as optimistic or pessimistic
            static const double MISM_THRESHOLD;

            // list of option names
            vector<string> m_opt_names;

            // count of options enabled by populations
            vector< vector<unsigned long> > m_opt_counts;

            // accumulate_stats
            void accumulate_stats(const chromosome & a_options, int a_pop_no);

            // application object that is target of this test
            const application & m_target;
            
            // listener
            acovea_listener & m_listener;
        
            // testing mode
            optimization_mode m_mode;
    };
    
    //----------------------------------------------------------
    // defines a collection of organisms and their ecology
    class acovea_world : protected common,
                         protected organism_factory< acovea_organism >,
                         protected landscape_factory< acovea_landscape >
    {
        public:
            // constructor
            acovea_world(acovea_listener & a_listener,
                         string a_bench_name,
                         optimization_mode a_mode,
                         const application & a_target,
                         size_t a_number_of_populations,
                         size_t a_population_size,
                         double a_survival_rate,
                         double a_migration_rate,
                         double a_mutation_rate,
                         double a_crossover_rate,
                         bool   a_use_scaling,
                         size_t a_generations);

            // destructor
            virtual ~acovea_world();

            // organism_factory methods
            acovea_organism create();

            void append(vector<acovea_organism> & a_population, size_t a_size);

            // landscape factory methods
            acovea_landscape generate();

            // run the algorithm to completion
            double run();
            
            // terminate run
            void terminate();

        private:
            // number of iterations to run
            const size_t m_generations;
            const string m_input_name;

            // create objects that define this population
            acovea_mutator                          m_mutator;
            acovea_reproducer                       m_reproducer;
            random_pool_migrator< acovea_organism > m_migrator;
            null_scaler< acovea_organism >          m_null_scaler;
            sigma_scaler< acovea_organism >         m_sigma_scaler;
            elitism_selector< acovea_organism >     m_selector;
            acovea_reporter                         m_reporter;

            // the evocosm that binds it all together
            evocosm< acovea_organism, acovea_landscape > * m_evocosm;

            // application object that is target of this test
            const application & m_target;
            
            // a listener to report events
            acovea_listener & m_listener;
            
            // optimization mode
            optimization_mode m_mode;
            
            // a string stream to hold the configuration text
            ostringstream m_config_text;
    };
}

#endif
