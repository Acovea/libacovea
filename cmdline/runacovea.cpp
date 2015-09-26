//---------------------------------------------------------------------
//  ACOVEA -- Analysis of Compiler Options Via Evolution Algorithm
//
//  runacovea.cpp
//
//  A driver program from the ACOVEA genetic algorithm.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
using namespace std;

#include "libcoyotl/command_line.h"
using namespace libcoyotl;

#include "libacovea/acovea.h"
using namespace acovea;

//----------------------------------------------------------
// display program options and command line
void show_usage()
{
    cout << "usage: runacovea -config {name} -input {name} [options]\n\n"
         << "essential options:\n"
         << "   -config {config file name}              (i.e., -config gcc34_opteron.acovea\n"
         << "   -input {source base name}               (i.e., -input almabench.c)\n"
         << "\noptions for tuning the evolutionary algorithm:\n"
         << "   -n {number of populations to create}\n"
         << "   -p {size of each population}\n"
         << "   -g {number of generations to run}\n"
         << "   -sr {survival rate}                     (between 0.0 and 1.0)\n"
         << "   -mr {mutation rate}                     (between 0.0 and 1.0)\n"
         << "   -cr {crossover rate}                    (between 0.0 and 1.0)\n"
         << "   -ir {immigration rate}                  (between 0.0 and 1.0)\n"
         << "   -no-scaling                             (no fitness scaling)\n"
         << "   -size                                   (optimize for code size)\n"
         << "   -retval                                 (optimize for return value)\n"
         << "   -seed {random number seed}\n\n"
         << "example:\n"
         << "   runacovea -config gcc34_opteron.acovea -input huffbench.c\n\n";
}
 
//----------------------------------------------------------
//  main program
int main(int argc, char * argv[])
{
    // display header
    cout << "\nAcovea " << ACOVEA_VERSION << " (compiled " << __DATE__ << " " << __TIME__ << ")"
         << "\nEvolving Better Software\n"
         << "\nInvented by Scott Robert Ladd         (scott.ladd@coyotegulch.com)"
         << "\n            Coyote Gulch Productions  (http://www.coyotegulch.com)\n";
            
    // parse command line
    set<string> bool_options; // empty list

    command_line args(argc,argv,bool_options);

    // settings    
    size_t number_of_pops  =   5;
    size_t population_size =  40;
    size_t generations     =  20;
    double survival_rate   =   0.10;
    double migration_rate  =   0.05;
    double mutation_rate   =   0.01;
    double crossover_rate  =   1.00;
    string input_name;
    string config_name;
    bool scaling           = true;
    optimization_mode mode = OPTIMIZE_SPEED;
    
    string id;
    
    for (vector<command_line::option>::const_iterator opt = args.get_options().begin(); opt != args.get_options().end(); ++opt)
    {
        if (opt->m_name == "n")
        {
            number_of_pops = atol(opt->m_value.c_str());
            
            if (number_of_pops < 1)
                number_of_pops = 1;
        }
        else if (opt->m_name == "p")
        {
            population_size = atol(opt->m_value.c_str());
            
            if (population_size < 2)
                population_size = 2;
        }
        else if (opt->m_name == "sr") // survival rate
        {
            survival_rate = atof(opt->m_value.c_str());
            
            if (survival_rate < 0.0)
                survival_rate = 0.0;
                
            if (survival_rate > 1.0)
                survival_rate = 1.0;
        }
        else if (opt->m_name == "ir") // survival rate
        {
            migration_rate = atof(opt->m_value.c_str());
            
            if (migration_rate < 0.0)
                migration_rate = 0.0;
                
            if (migration_rate > 0.9)
                migration_rate = 0.9;
        }
        else if (opt->m_name == "mr")
        {
            mutation_rate = atof(opt->m_value.c_str());
            
            if (mutation_rate < 0.0)
                mutation_rate = 0.0;
                
            if (mutation_rate > 0.95)
                mutation_rate = 0.95;
        }
        else if (opt->m_name == "cr")
        {
            crossover_rate = atof(opt->m_value.c_str());
            
            if (crossover_rate < 0.0)
                crossover_rate = 0.0;
                
            if (crossover_rate > 1.0)
                crossover_rate = 1.0;
        }
        else if (opt->m_name == "g")
        {
            generations = atol(opt->m_value.c_str());
            
            if (generations < 1)
                generations = 1;
        }
        else if (opt->m_name == "no-scaling")
        {
            scaling = false;
        }
        else if (opt->m_name == "size")
        {
            mode = OPTIMIZE_SIZE;
        }
        else if (opt->m_name == "retval")
        {
            mode = OPTIMIZE_RETVAL;
        }
        else if (opt->m_name == "seed")
        {
            libevocosm::globals::set_random_seed(atol(opt->m_value.c_str()));
        }
        else if (opt->m_name == "help")
        {
            show_usage();
            exit(0);
        }
        else if (opt->m_name == "input")
        {
            input_name = opt->m_value;
        }
        else if (opt->m_name == "config")
        {
            config_name = opt->m_value;
        }
        else
        {
            cout << "unknown option: " << opt->m_name << "\n\n";
            show_usage();
            exit(1);
        }
    }
    
    if ((config_name.length() == 0) || (input_name.length() == 0))
    {
        cerr << "You didn't specify an input or configuration, so here's some help.\n\n";
        show_usage();
        exit(1);
    }

    // verify existence of benchmark program
    /*
    struct stat input_stat;
    
    if (stat(bench_name.c_str(),&bench_stat))
    {
        string temp_name(ACOVEA_BENCHMARK_DIR);
        temp_name += bench_name;
        
        if (stat(temp_name.c_str(),&bench_stat))
        {
            cerr << "Could not find that benchmark, so here's some help.\n\n";
    	    show_usage();
            exit(1);
        }
        
        bench_name = temp_name;
    }
    */
    
    // create application object
    application target(config_name);
    
    // create a listener
    acovea_listener_stdout listener;
    
    // create a world
    acovea_world world(listener,
                       input_name,
                       mode,
                       target,
                       number_of_pops,
                       population_size,
                       survival_rate,
                       migration_rate,
                       mutation_rate,
                       crossover_rate,
                       scaling,
                       generations);

    try
    {    
        // run the world
        world.run();
    }
    catch (std::exception & ex)
    {
        cerr << "runacovea: " << ex.what() << "\n";
    }
    catch (...)
    {
        cerr << "runacovea: unknown exception\n";
    }
    
    // outa here
    return 0;
}
