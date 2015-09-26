//---------------------------------------------------------------------
//  ACOVEA -- Analysis of Compiler Options Via Evolution Algorithm
//
//  acovea.cpp
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

#include "acovea.h"
using namespace acovea;

#include "libcoyotl/realutil.h"
#include "libcoyotl/sortutil.h"
using namespace libcoyotl;

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
using namespace std;

// the expat XML parsing library
#include "expat.h"

// global constant
static const double BOGUS_RUN_TIME = 1000000000.0;

//----------------------------------------------------------
// abstract definition of a application option or switch

// creation constructor
option::option(bool a_enabled)
  : m_enabled(a_enabled)
{
    // nada
}

// copy constructor
option::option(const option & a_source)
  : m_enabled(a_source.m_enabled)
{
    // nada
}

// assignment 
option & option::operator = (const option & a_source)
{
    m_enabled = a_source.m_enabled;
    return *this;
}

// randomize settings of this option
void option::randomize()
{
    m_enabled = (g_random.get_rand_real2() < 0.5);
}

// mutate this option
void option::mutate()
{
    m_enabled = !m_enabled;
}

// virtual destructor (does nothing unless overridden)
option::~option()
{
    // nada
};

//----------------------------------------------------------
// an option that is just a string

// creation constructor
simple_option::simple_option(const string & a_name, bool a_enabled)
  : option(a_enabled),
    m_name(a_name)
{
    // nada
}

simple_option::simple_option(const char * a_name, bool a_enabled)
  : option(a_enabled),
    m_name(a_name)
{
    // nada
}

// copy constructor
simple_option::simple_option(const simple_option & a_source)
  : option(a_source),
    m_name(a_source.m_name)
{
    // nada
}

// assignment operator
simple_option & simple_option::operator = (const simple_option & a_source)
{
    option::operator = (a_source);
    m_name = a_source.m_name;
    return *this;
}

// clone
option * simple_option::clone()
{
    return new simple_option(*this);
}
            
//----------------------------------------------------------
// value settings tracker 

// constructor
tuning_settings_tracker::tuning_settings_tracker(const tuning_option & a_option)
  : m_values()
{
    // add value to list
    m_values.push_back(a_option.is_enabled() ? a_option.get_value() : 0);
}

// copy constructor
tuning_settings_tracker::tuning_settings_tracker(const tuning_settings_tracker & a_source)
  : m_values(a_source.m_values)
{
    // nada
}

// assignment
tuning_settings_tracker & tuning_settings_tracker::operator = (const tuning_settings_tracker & a_source)
{
    m_values = a_source.m_values;
}       

// get a string representing this settings tracker
string tuning_settings_tracker::get_settings_text()
{
    stringstream result;
    int average = 0;
    int  nonzero_count = 0;
    
    if (m_values.size() > 0)
    {
        for (vector<int>::iterator value = m_values.begin(); value != m_values.end(); ++value)
        {
            result << (*value) << " ";
            average += (*value);
            
            if ((*value) > 0)
                ++nonzero_count;
        }
    
        if (nonzero_count > 0)
            average /= nonzero_count;
        else
            average  = 0;
        
        result << ", average = " << average << " across " << nonzero_count << " populations";
    }
    
    return result.str();
}

// accumulate values
settings_tracker & tuning_settings_tracker::operator += (const settings_tracker & tracker)
{
    try
    {
        // this will crash is a non-value setting_tracker is the argument
        const vector<int> & new_values = (dynamic_cast<const tuning_settings_tracker &>(tracker)).m_values;
        
        // add new values to list
        for (vector<int>::const_iterator value = new_values.begin(); value != new_values.end(); ++value)
            m_values.push_back(*value);
    }
    catch (...)
    {
        cerr << "mixed tracker types in tuning_settings_tracker\n";
    }
    
    return *this;
}
        
//----------------------------------------------------------
// an option or switch that requires an integer argument

// creation constructor
tuning_option::tuning_option(const string & a_name,
                             bool a_enabled,
                             int  a_default,
                             int  a_min_value,
                             int  a_max_value,
                             int  a_step,
                             char a_separator)
  : simple_option(a_name, a_enabled),
    m_value(a_default),
    m_default(a_default),
    m_min_value(a_min_value),
    m_max_value(a_max_value),
    m_step(a_step),
    m_separator(a_separator)
{
    // make sure min < max
    if (m_min_value > m_max_value)
    {
        int temp = m_min_value;
        m_min_value = m_max_value;
        m_max_value = temp;
    }

    // step must be >= 1;
    if (m_step < 1)
        m_step = 1;
    
    // possibly adjust value to randomize populations
    size_t choice = g_random.get_rand_index(3);
    
    switch (choice)
    {
        case 0:
            m_value += m_step;
            break;
        case 1:
            m_value -= m_step;
            break;
    }

    // ensure value is inside specified range
    if (m_value < m_min_value)
        m_value = m_min_value;

    if (m_value > m_max_value)
        m_value = m_max_value;
}

// copy constructor
tuning_option::tuning_option(const tuning_option & a_source)
  : simple_option(a_source),
    m_value(a_source.m_value),
    m_default(a_source.m_default),
    m_min_value(a_source.m_min_value),
    m_max_value(a_source.m_max_value),
    m_step(a_source.m_step),
    m_separator(a_source.m_separator)
{
    // nada
}

// assignment operator
tuning_option & tuning_option::operator = (const tuning_option & a_source)
{
    simple_option::operator = (a_source);
    m_value       = a_source.m_value;
    m_default     = a_source.m_default;
    m_min_value   = a_source.m_min_value;
    m_max_value   = a_source.m_max_value;
    m_step        = a_source.m_step;
    m_separator   = a_source.m_separator;
    return *this;
}

// get the string for this option
string tuning_option::get() const
{
    stringstream result;
    result << m_name << m_separator << m_value;
    return result.str();
}

// mutate this option
void tuning_option::mutate()
{
    // select our mutation
    if (g_random.get_rand_real2() < 0.5)
        option::mutate();
    else
    {
        // mutate value of this option, up or down randomly
        if (g_random.get_rand_real2() < 0.5)
            m_value -= m_step;
        else
            m_value += m_step;

        // ensure value stays within bounds
        if (m_value < m_min_value)
            m_value = m_min_value;

        if (m_value > m_max_value)
            m_value = m_max_value;
    }
}

// clone
option * tuning_option::clone()
{
    return new tuning_option(*this);
}

//----------------------------------------------------------
// an option or switch set to one of several mutually-exclusive states
// creation constructor
enum_option::enum_option(const vector<string> & a_choices, bool a_enabled)
  : option(a_enabled),
    m_choices(a_choices),
    m_setting(g_random.get_rand_index(a_choices.size()))    
{
    // nada
}

// creation constructor
enum_option::enum_option(const char ** a_choices, size_t a_num_choices, bool a_enabled)
  : option(a_enabled),
    m_choices(),
    m_setting(g_random.get_rand_index(a_num_choices))    
{
    for (int n = 0; n < a_num_choices; ++n)
        m_choices.push_back(string(a_choices[n]));
}

// creation constructor
enum_option::enum_option(const char * a_choices, bool a_enabled)
  : option(a_enabled),
    m_choices(),
    m_setting(0)    
{
    // start tokenizing
    char * choices = strdup(a_choices);
    char * token   = strtok(choices,"|");

    while (token != NULL)
    {
        // add token to choice list
        m_choices.push_back(string(token));

        // next token
        token = strtok(NULL,"|");
    }
    
    m_setting = g_random.get_rand_index(m_choices.size());
    
    free(choices);
}

// copy constructor
enum_option::enum_option(const enum_option & a_source)
  : option(a_source),
    m_choices(a_source.m_choices),
    m_setting(a_source.m_setting)
{
    // nada
}

// assignment 
enum_option & enum_option::operator = (const enum_option & a_source)
{
    option::operator = (a_source);
    m_choices = a_source.m_choices;
    m_setting = a_source.m_setting;
}

// clone
option * enum_option::clone()
{
    return new enum_option(*this);
}

// get the string for this option
string enum_option::get() const
{
    return m_choices[m_setting];
}

// randomize settings of this option
void enum_option::randomize()
{
    // randomize enabled
    m_enabled = (g_random.get_rand_real2() < 0.5);
    
    // randomize setting
    m_setting = g_random.get_rand_index(m_choices.size());
}

// mutate this option
void enum_option::mutate()
{
    // select our mutation
    if (g_random.get_rand() & 1)
        option::mutate();
    else
    {
        // mutate setting of this option
        if (m_choices.size() == 2)
        {
            if (m_setting == 0)
                m_setting = 1;
            else
                m_setting = 0;
        }
        else
        {
            int new_setting = m_setting;
        
            // find a different setting
            while (new_setting == m_setting)
                new_setting = g_random.get_rand_index(m_choices.size());
            
            m_setting = new_setting;
        }
    }
}

// virtual destructor (does nothing unless overridden)
enum_option::~enum_option()
{
    // nada
}

//----------------------------------------------------------
// a vector of options

// constructor
chromosome::chromosome()
{
    // nada
}

// copy constructor
chromosome::chromosome(const chromosome & a_source)
{
    // add elements from source
    for (int n = 0; n < a_source.size(); ++n)
        push_back(a_source[n]->clone());
}

// assignment
chromosome & chromosome::operator = (const chromosome & a_source)
{
    // remove all elements from the vector
    clear();
    
    // add elements from source
    for (int n = 0; n < a_source.size(); ++n)
        push_back(a_source[n]->clone());
    
    return *this;
}

// destructor
chromosome::~chromosome()
{
    // free memory allocated in elements of vector
    for (vector<option *>::iterator opt = begin(); opt != end(); ++opt)
        delete *opt;
}

//----------------------------------------------------------
// the definition of a application

// static functions used by XML parser
static void parser_start(void * compiler_ptr, const char * element, const char ** attr)
{
    // have the application process this element
    static_cast<application *>(compiler_ptr)->import_element(element, attr);
}

static void parser_end(void * compiler_ptr, const char * element)
{
    // nada
}
            
// import an XML element
void application::import_element(const char * element, const char ** attr)
{
    int i;
    const char * value = NULL;
    const char * type = NULL;
    int defval  = 0;
    int minval  = 0;
    int maxval  = 0;
    int stepval = 0;
    char sep    = '=';
    
    if (0 == strcmp(element,"description"))
    {
        for (i = 0; attr[i] != NULL; i += 2)
        {
            if (0 == strcmp(attr[i],"value"))
                m_description = string(attr[i + 1]);
            else if (0 == strcmp(attr[i],"version"))
                m_config_version = string(attr[i + 1]);
        }
    }
    if (0 == strcmp(element,"get_version"))
    {
        for (i = 0; attr[i] != NULL; i += 2)
        {
            if (0 == strcmp(attr[i],"value"))
                m_get_app_version = string(attr[i + 1]);
        }
    }
    else if (0 == strcmp(element,"quoted_options"))
    {
        for (i = 0; attr[i] != NULL; i += 2)
        {
            if (0 == strcmp(attr[i],"value"))
            {
                if (0 == strcmp(attr[i + 1],"true"))
                    m_quoted_options = true;
                else
                    m_quoted_options = false;
            }
        }
    }
    else if (0 == strcmp(element,"prime"))
    {
        m_prime.m_description = "Prime";
        
        for (i = 0; attr[i] != NULL; i += 2)
        {
            if (0 == strcmp(attr[i],"command"))
                m_prime.m_command = attr[i + 1];
            else if (0 == strcmp(attr[i],"flags"))
                m_prime.m_flags = attr[i + 1];
        }
    }
    else if (0 == strcmp(element,"baseline"))
    {
        command_elements baseline;
        
        for (i = 0; attr[i] != NULL; i += 2)
        {
            if (0 == strcmp(attr[i],"command"))
                baseline.m_command = attr[i + 1];
            else if (0 == strcmp(attr[i],"description"))
                baseline.m_description = attr[i + 1];
            else if (0 == strcmp(attr[i],"flags"))
                baseline.m_flags = attr[i + 1];
        }
        
        m_baselines.push_back(baseline);
    }
    else if (0 == strcmp(element,"flag"))
    {
        // search attributes
        for (i = 0; attr[i] != NULL; i += 2)
        {
            if (0 == strcmp(attr[i],"value"))
                value = attr[i + 1];
            else if (0 == strcmp(attr[i],"type"))
                type = attr[i + 1];
            else if (0 == strcmp(attr[i],"default"))
                defval = atoi(attr[i + 1]);
            else if (0 == strcmp(attr[i],"min"))
                minval = atoi(attr[i + 1]);
            else if (0 == strcmp(attr[i],"max"))
                maxval = atoi(attr[i + 1]);
            else if (0 == strcmp(attr[i],"step"))
                stepval = atoi(attr[i + 1]);
            else if (0 == strcmp(attr[i],"separator"))
                sep = attr[i + 1][0];
        }
        
        if ((value != NULL) && (type != NULL))
        {
            if (0 == strcmp(type,"simple"))
                m_options.push_back(new simple_option(value,false));
            else if (0 == strcmp(type,"enum"))
                m_options.push_back(new enum_option(value,false));
            else if (0 == strcmp(type,"tuning"))
                m_options.push_back(new tuning_option(value,false,defval,minval,maxval,stepval,sep));
        }
    }
    else
    {
        // ignore anything we don't understand, like George W. Bush
    }
}

// creation constructor
application::application(const string & a_config_name)
  : m_config_name(a_config_name),
    m_prime(),
    m_baselines(),
    m_description(),
    m_options()
{
    // create an XML parser
	XML_Parser parser = XML_ParserCreate(NULL); 
    
    if (!parser)
        throw runtime_error("unable to create XML parser");
    
    // set the "user data" for the parser to this application
    XML_SetUserData(parser,static_cast<void *>(this));
    
    // set the element handler
    XML_SetElementHandler(parser,parser_start,parser_end);
    
    // read the input file to a buffer
    char * xml_buffer = new char[65538];
    
    FILE * xml_file = fopen(m_config_name.c_str(),"r");
    
    if (xml_file == NULL)
    {
        // try opening it in the source directory
        string temp_name(ACOVEA_CONFIG_DIR);
        temp_name += m_config_name; 
                
	xml_file = fopen(temp_name.c_str(),"r");

	if (xml_file == NULL)
            throw runtime_error("unable to open configuration file");
    }
    
    long nread = fread(xml_buffer,1,65538,xml_file);
    
    if (ferror(xml_file))
        throw runtime_error("unable to read from configuration file");

    // we're done with the file    
    fclose(xml_file);
    
    // parse the file and report any error    
    if (!XML_Parse(parser,xml_buffer,nread,1))
    {
        /*
        stderr << "invalid configuration file\nerror at line "
               << XML_GetCurrentLineNumber(parser)
               << "\n"
               << XML_ErrorString(XML_GetErrorCode(parser))
               << "\n"
        */
                
        throw runtime_error("XML parsing error");
    }
    
    // release memory
    delete [] xml_buffer;
}

// copy constructor
application::application(const application & a_source)
  : m_config_name(a_source.m_config_name),
    m_prime(a_source.m_prime),
    m_baselines(a_source.m_baselines),
    m_description(a_source.m_description),
    m_options(a_source.m_options)
{
    // nada
}

// assignment
application & application::operator = (const application & a_source)
{
    // call base class operator
    application::operator = (a_source);
    
    // assign members
    m_config_name = a_source.m_config_name;
    m_prime       = a_source.m_prime;
    m_baselines   = a_source.m_baselines;
    m_description = a_source.m_description;
    m_options     = a_source.m_options;

    return *this;
}

// get application configuration in XML
void application::get_xml(ostream & a_stream) const
{
}

// return a string description of the application and the configuration
string application::get_description() const
{
    return m_description;
}

// set the description string
void application::set_description(const string & a_description)
{
    m_description = a_description;
}

// return a string description of the application and the configuration
string application::get_config_version() const
{
    return m_config_version;
}

// set the version string
void application::set_config_version(const string & a_config_version)
{
    m_config_version = a_config_version;
}

// return the name of the configuration file used
string application::get_config_name() const
{
    return m_config_name;
}

// set configuration name
void application::set_config_name(const string & a_config_name)
{
    m_config_name = a_config_name;
}

// return the command for invoking the application
command_elements application::get_prime() const
{
    return m_prime;
}

// return the flags elements
vector<command_elements> application::get_baselines() const
{
    return m_baselines;
}

// get option list
chromosome application::get_options() const
{
    return m_options;
}

// set option list
void application::set_options(const chromosome & a_options)
{
    m_options = a_options;
}

// get application name
string application::get_app_name() const
{
    return m_prime.m_command;
}
            
// get the get version command
vector<string> application::get_get_app_version() const
{
    // start with the command head
    vector<string> command;
    
    char * v = strdup(m_get_app_version.c_str());
    char * token = strtok(v," ");

    while (token != NULL)
    {
        string::size_type pos;
        string token_s(token);
        command.push_back(token_s);
        token = strtok(NULL," ");
    }

    free(v);

    // return complete command set
    return command;
}

// return an argument list for compiling a given program
vector<string> application::get_prime_command(const string &     a_input_name,
                                              const string &     a_output_name,
                                              const chromosome & a_options) const
{
    return get_command(m_prime,a_input_name,a_output_name,a_options);
}

vector<string> application::get_command(const command_elements & a_elements,
                                        const string &           a_input_name,
                                        const string &           a_output_name,
                                        const chromosome &       a_options) const
{
    // start with the command head
    vector<string> command;
    
    command.push_back(a_elements.m_command);

    // add enabled options
    // add flags elements
    static const string ACOVEA_INPUT("ACOVEA_INPUT");
    static const string ACOVEA_OUTPUT("ACOVEA_OUTPUT");
    static const string ACOVEA_OPTIONS("ACOVEA_OPTIONS");
    
    char * v = strdup(a_elements.m_flags.c_str());
    char * token = strtok(v," ");

    while (token != NULL)
    {
        string::size_type pos;
        string token_s(token);
        bool push_token = true;
        
        pos = token_s.find(ACOVEA_INPUT);
        if (pos != string::npos)
            token_s.replace(pos,ACOVEA_INPUT.length(),a_input_name);

        pos = token_s.find(ACOVEA_OUTPUT);
        if (pos != string::npos)
            token_s.replace(pos,ACOVEA_OUTPUT.length(),a_output_name);

        pos = token_s.find(ACOVEA_OPTIONS);
        if (pos != string::npos)
        {
            if (m_quoted_options)
            {
                string options;

                for (int n = 0; n < a_options.size(); ++n)
                {
                    if (a_options[n]->is_enabled())
                        options += a_options[n]->get() + " ";
                }
                
                if (options.length() < 1)
                    token_s.replace(pos,ACOVEA_OPTIONS.length(),string(""));
                else
                    token_s.replace(pos,ACOVEA_OPTIONS.length(),options);
            }
            else
            {
                for (int n = 0; n < a_options.size(); ++n)
                {
                    
                    if (a_options[n]->is_enabled())
                        command.push_back(a_options[n]->get());
                    
                    push_token = false;
                }
            }
        }

        if (push_token)
            command.push_back(token_s);

        token = strtok(NULL," ");
    }

    free(v);

    // return complete command set
    return command;
}

// get a random set of options for this application
chromosome application::get_random_options() const
{
    int n;
    
    // result
    chromosome options;
    
    // push_back options
    for (n = 0; n < m_options.size(); ++n)
        options.push_back(m_options[n]->clone());
    
    for (n = 0; n < m_options.size(); ++n)
	    options[n]->randomize();
    
    // done
    return options;
}

// breed a new options set from two parents
chromosome application::breed(const chromosome & a_parent1,
                           const chromosome & a_parent2) const
{
    // This function assumes that the two lists are the same length, and
    // contain the same list of options in the same order
    if (a_parent1.size() != a_parent2.size())
    {
        char message[128];
        snprintf(message,128,"incompatible option vectors in breeding (sizes %d and %d)",a_parent1.size(),a_parent2.size());
        throw invalid_argument(message);
    }
    
    // result
    chromosome child;
    
    // randomly pick an option from one of the parents
    for (int n = 0; n < a_parent1.size(); ++n)
    {
        if (g_random.get_rand() & 1)
            child.push_back(a_parent1[n]->clone());
        else
            child.push_back(a_parent2[n]->clone());
    }
    
    // done
    return child;
}

// mutate an option set
void application::mutate(chromosome & a_options,
                            double a_mutation_chance) const
{
    for (int n = 0; n < a_options.size(); ++n)
    {
        if (g_random.get_rand_real2() < a_mutation_chance)
            a_options[n]->mutate();
    }
}

// get the option set size (should be fized for all chromosomes created
//   by this application)
size_t application::chromosome_length() const
{
    return m_options.size();
}
            
// destructor (to support derived classes)
application::~application()
{
    // nada
}

//----------------------------------------------------------
// the organism undergoing evolution
acovea_organism::acovea_organism()
  : organism< chromosome >()
{
    // nada
}

acovea_organism::acovea_organism(const application & a_target)
  : organism< chromosome >(a_target.get_random_options())
{
    // nada
}

acovea_organism::acovea_organism(const application & a_target,
                                 const chromosome & a_genes)
  : organism< chromosome >(a_genes)
{
    // nada
}

acovea_organism::acovea_organism(const acovea_organism & a_source)
  : organism< chromosome >(a_source)
{
    // nada
}

acovea_organism::acovea_organism(const acovea_organism & a_parent1,
                                 const acovea_organism & a_parent2,
                                 const application & a_target)
  : organism< chromosome >()
{
    m_genes = a_target.breed(a_parent1.genes(),a_parent2.genes());
}

acovea_organism::~acovea_organism()
{
    // nada
}

acovea_organism & acovea_organism::operator = (const acovea_organism & a_source)
{
    organism< chromosome >::operator = (a_source);
    return *this;
}

//----------------------------------------------------------
// mutation operator
acovea_mutator::acovea_mutator(double a_mutation_rate, const application & a_target)
  : m_mutation_rate(a_mutation_rate),
    m_target(a_target)
{
    // adjust mutation rate if necessary
    if (m_mutation_rate >= 0.95)
        m_mutation_rate = 0.95;
    else if (m_mutation_rate < 0.0)
        m_mutation_rate = 0.0;
}

// copy constructor
acovea_mutator::acovea_mutator(const acovea_mutator & a_source)
  : m_mutation_rate(a_source.m_mutation_rate),
    m_target(a_source.m_target)
{
    // nada
}

// destructor
acovea_mutator::~acovea_mutator()
{
    // nada
}

// assignment -- note, this can not change the target!
acovea_mutator & acovea_mutator::operator = (const acovea_mutator & a_source)
{
    m_mutation_rate = a_source.m_mutation_rate;
    return *this;
}

// mutation
void acovea_mutator::mutate(vector< acovea_organism > & a_population)
{
    for (vector< acovea_organism >::iterator org = a_population.begin(); org != a_population.end(); ++org)
        m_target.mutate(org->genes(),m_mutation_rate);
}

//----------------------------------------------------------
// reproduction operator
// creation constructor
acovea_reproducer::acovea_reproducer(double a_crossover_rate, const application & a_target)
  : m_crossover_rate(a_crossover_rate),
    m_target(a_target)
{
    // adjust crossover rate if necessary
    if (m_crossover_rate > 1.0)
        m_crossover_rate = 1.0;
    else if (m_crossover_rate < 0.0)
        m_crossover_rate = 0.0;
}

// copy constructor
acovea_reproducer::acovea_reproducer(const acovea_reproducer & a_source)
  : m_crossover_rate(a_source.m_crossover_rate),
    m_target(a_source.m_target)
{
    // nada
}

// destructor
acovea_reproducer::~acovea_reproducer()
{
    // nada
}

// assignment; can't change target (which is a ref)
acovea_reproducer & acovea_reproducer::operator = (const acovea_reproducer & a_source)
{
    m_crossover_rate = a_source.m_crossover_rate;
    return *this;
}

// interrogator
vector<acovea_organism> acovea_reproducer::breed(const vector< acovea_organism > & a_population,
                                                 size_t a_limit)
{
    // result
    vector< acovea_organism > children;

    if (a_limit > 0U)
    {
        // construct a fitness wheel
        vector<double> wheel_weights;

        for (vector< acovea_organism >::const_iterator org = a_population.begin(); org != a_population.end(); ++org)
            wheel_weights.push_back(org->fitness());

        roulette_wheel fitness_wheel(wheel_weights);

        // create children
        while (a_limit > 0)
        {
            // clone an existing organism as a child
            size_t first_index = fitness_wheel.get_index();
            acovea_organism * child;

            // do we crossover?
            if (g_random.get_rand_real2() <= m_crossover_rate)
            {
                // select a second parent
                size_t second_index = first_index;

                while (second_index == first_index)
                    second_index = fitness_wheel.get_index();

                // reproduce
                child = new acovea_organism(m_target,
                                            m_target.breed(a_population[first_index].genes(),
                                                           a_population[second_index].genes()));
            }
            else
                // no crossover; just copy first organism chosen
                child = new acovea_organism(a_population[first_index]);

            // add child to new population
            children.push_back(*child);
            delete child;

            // one down, more to go?
            --a_limit;
        }
    }

    // outa here!
    return children;
}

//----------------------------------------------------------
// fitness landscape

acovea_landscape::acovea_landscape(string a_bench_name,
                                   optimization_mode a_mode,
                                   const application & a_target,
                                   acovea_listener & a_listener)
    : landscape<acovea_organism>(a_listener),
      m_input_name(a_bench_name),
      m_target(a_target),
      m_mode(a_mode)
{
    // nada
}

acovea_landscape::acovea_landscape(const acovea_landscape & a_source)
    : landscape<acovea_organism>(a_source),
      m_input_name(a_source.m_input_name),
      m_target(a_source.m_target),
      m_mode(a_source.m_mode)
{
    // nada
}

acovea_landscape & acovea_landscape::operator = (const acovea_landscape & a_source)
{
    landscape<acovea_organism>::operator = (a_source);
    m_input_name = a_source.m_input_name;
    m_mode = a_source.m_mode;
    // can't duplicate m_target since it's a reference
    return *this;
}

acovea_landscape::~acovea_landscape()
{
    // nada
}

static double run_test(const vector<string> & command,
                       string temp_name,
                       listener & listener,
                       optimization_mode mode)
{
    // resulting fitness
    double fitness = 0.0;
    
    // allocate array of string pointers for exec
    char ** argv = new char * [command.size() + 1];
    
    // create string representing the command
    string command_text;

    // fill list and string versions of command    
    for (int n = 0; n < command.size(); ++n)
    {
        // dupe so we know the string doesn't go away
        argv[n] = strdup(command[n].c_str());
        command_text += command[n] + " ";
    }
    
    // terminate argument list
    argv[command.size()] = NULL;

    // create compile process and wait for it to finish
    pid_t child_pid;
    int child_retval;
    child_pid = fork();

    if (child_pid == 0)
        execvp(argv[0],argv);

    while (0 == waitpid(child_pid,&child_retval,WNOHANG))
        listener.yield();
    
    // we're done compiling; clean up string dupes!
    for (int n = 0; n < command.size(); ++n)
        free(argv[n]);

    // make sure compile succeeded before running program
    if (child_retval == 0)
    {
        if (mode == OPTIMIZE_SIZE)
        {
            // get resulting file size and use as fitness
            struct stat stats;
            stat(temp_name.c_str(),&stats);
            fitness = (double)stats.st_size;
        }
        else // OPTIMIZE_SPEED or OPTIMIZE_RETVAL
        {
            // run the program
            argv[0] = strdup(temp_name.c_str());
            argv[1] = "-ga";
            argv[2] = NULL;

            // constants for I/O descriptors
            static const int PIPE_IN  = 0;
            static const int PIPE_OUT = 1;

            // create pipe
            int fds[2];
            pipe(fds);

            // fork and exec program
            child_pid = fork();

            if (child_pid == 0)
            {
                // redirect std. output for child
                close(STDOUT_FILENO);
                dup2(fds[PIPE_OUT],STDOUT_FILENO);
                close(fds[PIPE_IN]);
                close(fds[PIPE_OUT]);

                execve(temp_name.c_str(),argv,NULL);
            }

            // redirect std. input for parent
            close(STDIN_FILENO);
            dup2(fds[PIPE_IN],STDIN_FILENO);
            close(fds[PIPE_IN]);
            close(fds[PIPE_OUT]);

            // wait for child to finish
            while (0 == waitpid(child_pid,&child_retval,WNOHANG))
                listener.yield();
            
            if (mode == OPTIMIZE_SPEED)
            {
                if (child_retval == 0)
                {
                    // read run time 
                    double run_time = 0.0;
                    char temp[32];
                    fgets(temp,32,stdin);
                    run_time = atof(temp);

                    // free memory
                    free(argv[0]);

                    // record fitness
                    fitness = run_time;
                }
                else
                {
                    // handle application error
                    ostringstream errormsg;
                    errormsg << "\nRUN FAILED:\n" << command_text << endl;
                    listener.report_error(errormsg.str());
                    fitness = BOGUS_RUN_TIME;
                }
            }
            else // OPTIMIZE_RETVAL
            {
                fitness = (double)child_retval;
            }
        }
    }
    else
    {
        // handle application error
        ostringstream errormsg;
        errormsg << "\nCOMPILE FAILED:\n" << command_text << endl;
        listener.report_error(errormsg.str());
        fitness = BOGUS_RUN_TIME;
    }
    
    // remove temporary file
    remove(temp_name.c_str());

    // done
    return fitness;
}

static string get_temp_name()
{
    // generate a unique file name
    uint32_t file_code = 0;     
    
    // first try to read /dev/urandom
    int fd = open ("/dev/urandom", O_RDONLY);

    if (fd != -1)
    {
        read(fd, &file_code, 4);
        close(fd);
    }
    else
        file_code = (uint32_t)time(NULL);
    
    char temp_name[32];
    snprintf(temp_name,32,"/tmp/ACOVEA%08X",file_code);
    return string(temp_name);    
}
        
double acovea_landscape::test(acovea_organism & a_org, bool a_verbose) const
{
    // run a test
    string temp_name = get_temp_name();
    
    a_org.fitness() = run_test(m_target.get_prime_command(m_input_name,temp_name,a_org.genes()),
                               temp_name,
                               m_listener,
                               m_mode);

    // done
    return a_org.fitness();
}

// fitness testing
double acovea_landscape::test(vector< acovea_organism > & a_population) const
{
    double result = 0.0;
    size_t n = 0;

    // test each org
    for (vector< acovea_organism >::iterator org = a_population.begin(); org != a_population.end(); ++org)
    {
        ++n;
        m_listener.ping_fitness_test_begin(n);
        result += test(*org);
        m_listener.ping_fitness_test_end(n);
        m_listener.yield();
    }

    // done; return average population fitness
    return result / a_population.size();
}

//----------------------------------------------------------
// status and statistics

// the threshold for reporting an option as optimistic or pessimistic
const double acovea_reporter::MISM_THRESHOLD = 1.5;

acovea_reporter::acovea_reporter(string a_bench_name,
                                 size_t a_number_of_populations,
                                 const application & a_target,
                                 acovea_listener & a_listener,
                                 optimization_mode a_mode)
  : reporter<acovea_organism,acovea_landscape>(a_listener),
    m_input_name(a_bench_name),
    m_number_of_populations(a_number_of_populations),
    m_target(a_target),
    m_opt_names(),
    m_opt_counts(),
    m_listener(a_listener),
    m_mode(a_mode)
{
    // we don't care about settings, just what they're named
    chromosome options = a_target.get_random_options();
    
    // allocate and initialize option names and counts
    for (int n = 0; n < options.size(); ++n)
    {
        vector<string> choices = options[n]->get_choices();
        
        for (int i = 0; i < choices.size(); ++i)
        {
            m_opt_names.push_back(choices[i]);
        
            m_opt_counts.push_back(vector<unsigned long>(m_number_of_populations + 1));
                    
            for (int p = 0; p <  m_number_of_populations + 1; ++p)
                m_opt_counts[n + i][p] = 0UL;
        }
    }
}

acovea_reporter::~acovea_reporter()
{
    // nada
}

       
// accumulate counts of options
void acovea_reporter::accumulate_stats(const chromosome & a_options, int a_pop_no)
{
    // increment totals
    int n = 0;
    
    for (int i = 0; i < a_options.size(); ++i)
    {
        vector<string> choices = a_options[i]->get_choices();
        
        if (a_options[i]->is_enabled()) 
        {
            if (choices.size() == 1)
            {
                if (a_pop_no >= 0)
                {
                    ++m_opt_counts[n][a_pop_no];
                    ++m_opt_counts[n][m_number_of_populations];
                }
            }
            else
            {
                if (a_pop_no >= 0)
                {
                    ++m_opt_counts[n + a_options[i]->get_setting()][a_pop_no];
                    ++m_opt_counts[n + a_options[i]->get_setting()][m_number_of_populations];
                }
            }
        }
        
        n += choices.size();
    }
}

bool acovea_reporter::report(const vector< vector< acovea_organism > > & a_populations,
                             size_t a_iteration,
                             double & a_fitness,
                             bool a_finished)
{
    // exit if populations is empty
    if (a_populations.size() < 1U)
        return false;

    // find best organisms for each population
    acovea_organism * best_one  = new acovea_organism[a_populations.size()];
    acovea_organism best_of_best;
    
    // long times are poor fitness, so we start impossibly big
    for (int p = 0; p < m_number_of_populations; ++p)
        best_one[p].fitness() = BOGUS_RUN_TIME; 
    
    double avg_count   = 0.0;
    double avg_fitness = 0.0;
    
    // check each population and find smallest fitness (= shortest run time)
    for (int p = 0; p < m_number_of_populations; ++p)
    {
        for (vector< acovea_organism >::const_iterator org = a_populations[p].begin(); org != a_populations[p].end(); ++org)
        {
            if (org->fitness() < best_one[p].fitness())
            {
                best_of_best = *org;
                best_one[p] = *org;
            }

            if (org->fitness() != BOGUS_RUN_TIME)
            {
                avg_count += 1.0;
                avg_fitness += org->fitness();
            }
        }
            
        // accumulate stats based on best organism
        accumulate_stats(best_one[p].genes(),p);
    }
    
    // compute average fitness for all organisms
    avg_fitness /= avg_count;
    
    // display report for this generation
    m_listener.report_generation(a_iteration,avg_fitness);
    
    // report final statistics
    if (a_finished)
    {
        // compile results
        vector<test_result> tests;

        // calculate the mean
        double mean = 0.0;
        
        for (int i = 0; i < m_opt_names.size(); ++i)
            mean += static_cast<double>(m_opt_counts[i][m_number_of_populations]);

        mean /= static_cast<double>(m_opt_names.size());
        
        // calculate variance
        double variance = 0.0;
        
        for (int i = 0; i < m_opt_names.size(); ++i)
        {
            double diff = static_cast<double>(m_opt_counts[i][m_number_of_populations]) - mean;
            variance += (diff * diff);
        }

        variance /= static_cast<double>(m_opt_names.size());

        // calculate the std. deviation (sigma)
        double sigma = sqrt(variance);

        // calculate zscores and report
        vector<option_zscore> zscores;
        
        option_zscore oz;
        
        for (int i = 0; i < m_opt_names.size(); ++i)
        {
            oz.m_name   = m_opt_names[i];
            oz.m_zscore = sigdig(((static_cast<double>(m_opt_counts[i][m_number_of_populations]) - mean) / sigma),4);
            zscores.push_back(oz);
        }
        
        // only display common options if more than one population
        if (m_number_of_populations > 1)
        {
            // using best fitnesses, compute "common" genes for multiple populations
            //chromosome optopt_options = best_of_best.genes();
            chromosome common_options = best_of_best.genes();
            chromosome empty_options  = best_of_best.genes();

            for (int n = 0, n2 = 0; n < empty_options.size(); ++n, ++n2)
            {
                // optopt_options[n]->set_enabled(zscores[n].m_zscore >= MISM_THRESHOLD);
                empty_options[n]->set_enabled(false);
            }
            
            for (int p = 0; p < m_number_of_populations; ++p)
            {
                // keep shared genes
                if (p != 0)
                {
                    chromosome temp = best_one[p].genes();

                    for (int n = 0; n < common_options.size(); ++n)
                        common_options[n]->set_enabled(common_options[n]->is_enabled() & temp[n]->is_enabled());
                }
            }

            // for common tuning option, average the assigned values
            /*
            for (int n = 0; n < common_options.size(); ++n)
            {
                if (common_options[n]->is_enabled() & common_options[n]->has_settings())
                {
                    cout << "tuning option" << endl;
                    long sum = 0;
                    for (int p = 0; p < m_number_of_populations; ++p)
                        sum += dynamic_cast<const tuning_option *>(a_populations[p][n])->get_value();

                    // set common option to average
                    dynamic_cast<tuning_option *>(common_options[n])->set_value(sum / m_number_of_populations);
                }
            }
            */

            // get application strings
            //string optopt_temp_name = get_temp_name();
            string bestof_temp_name = get_temp_name();
            string common_temp_name = get_temp_name();
            
            //vector<string> optopt_command = m_target.get_prime_command(m_input_name,optopt_temp_name,optopt_options);
            vector<string> bestof_command = m_target.get_prime_command(m_input_name,bestof_temp_name,best_of_best.genes());
            vector<string> common_command = m_target.get_prime_command(m_input_name,common_temp_name,common_options);
            
            //test_result optopt_result = { "Acovea's Optimistic Options", string(), 0.0, true };
            test_result bestof_result = { "Acovea's Best-of-the-Best", string(), 0.0, true };
            test_result common_result = { "Acovea's Common Options", string(), 0.0, true };
            
            //optopt_result.m_fitness = run_test(optopt_command,optopt_temp_name,m_listener,m_mode);
            bestof_result.m_fitness = run_test(bestof_command,bestof_temp_name,m_listener,m_mode);
            common_result.m_fitness = run_test(common_command,common_temp_name,m_listener,m_mode);

            //for (int n = 0; n < optopt_command.size(); ++n)
            //    optopt_result.m_detail += optopt_command[n] + " ";

            for (int n = 0; n < bestof_command.size(); ++n)
                bestof_result.m_detail += bestof_command[n] + " ";

            for (int n = 0; n < common_command.size(); ++n)
                common_result.m_detail += common_command[n] + " ";

            //tests.push_back(optopt_result);
            tests.push_back(bestof_result);
            tests.push_back(common_result);
            
            // add the baselines
            vector<command_elements> baselines = m_target.get_baselines();
            
            for (int n = 0; n < baselines.size(); ++n)
            {
                test_result result;
                string      temp_name = get_temp_name();
                
                result.m_description      = baselines[n].m_description;
                result.m_acovea_generated = false;
                vector<string> command    = m_target.get_command(baselines[n],m_input_name,temp_name,empty_options);
                result.m_fitness          = run_test(command,temp_name,m_listener,m_mode);
                
                for (int n = 0; n < command.size(); ++n)
                    result.m_detail += command[n] + " ";
                
                tests.push_back(result);
            }
        }

        // send results to listener
        m_listener.report_final(tests,zscores);
    }

    // clean up    
    delete [] best_one;
    
    return true;
}

void acovea_listener_stdout::ping_generation_begin(size_t a_generation_number)
{
    cout << "------------------------------------------------------------\ngeneration "
         << a_generation_number << " begins" << endl;
}

void acovea_listener_stdout::ping_generation_end(size_t a_generation_number)
{
    // nada
}

void acovea_listener_stdout::ping_population_begin(size_t a_population_number)
{
    cout << "\npopulation " << setw(2) << a_population_number << ": " << flush;
}

void acovea_listener_stdout::ping_population_end(size_t a_population_number)
{
    // nada
}

void acovea_listener_stdout::ping_fitness_test_begin(size_t a_organism_number)
{
    // nada
}

void acovea_listener_stdout::ping_fitness_test_end(size_t a_organism_number)
{
    cout << "." << flush;
}

void acovea_listener_stdout::report(const string & a_text)
{
    cout << a_text;
}

void acovea_listener_stdout::report_error(const string & a_text)
{
    cerr << a_text;
}

void acovea_listener_stdout::run_complete()
{
    // nada
}

void acovea_listener_stdout::yield()
{
    usleep(50000);
}

void acovea_listener_stdout::report_config(const string & a_text)
{
    cout << a_text;
}

void acovea_listener_stdout::report_generation(size_t a_gen_no, double  a_avg_fitness)
{
    cout << "\n\ngeneration " << a_gen_no
         << " complete, average fitness: " << a_avg_fitness
         << endl;
}

void acovea_listener_stdout::report_final(vector<test_result> & a_results, vector<option_zscore> & a_zscores)
{
    static const double THRESHOLD = 1.5;
    
    // format time
    char time_text[256];
    time_t now = time(NULL);
    strftime(time_text,256,"%Y %b %d %X",localtime(&now));

    cout << "\nAcovea completed its analysis at " << time_text << endl;
    
    // report optimistic options
    bool flag = false;
        
    cout << "\nOptimistic options:\n\n";
    for (int n = 0; n < a_zscores.size(); ++n)
    {
        if (a_zscores[n].m_zscore >= THRESHOLD)
        {
            flag = true;
            
            cout << right << setw(40) << a_zscores[n].m_name
                 << "  (" << a_zscores[n].m_zscore << ")\n";
        }
    }
        
    if (!flag)
        cout << "        none" << endl;
    
    flag = false;
        
    cout << "\nPessimistic options:\n\n";
    for (int n = 0; n < a_zscores.size(); ++n)
    {
        if (a_zscores[n].m_zscore <= -THRESHOLD)
        {
            flag = true;
            
            cout << right << setw(40) << a_zscores[n].m_name
                 << "  (" << a_zscores[n].m_zscore << ")\n";
        }
    }
        
    if (!flag)
        cout << "        none" << endl;
    
    // graph test results
    double big_fit = numeric_limits<double>::min();
        
    for (int n = 0; n < a_results.size(); ++n)
    {
        cout << "\n" << a_results[n].m_description << ":\n"
             << a_results[n].m_detail
             << endl;

        if (a_results[n].m_fitness > big_fit)
            big_fit = a_results[n].m_fitness;
    }
    
    cout << "\n\nA relative graph of fitnesses:\n";
    
    for (int n = 0; n < a_results.size(); ++n)
    {
        cout << "\n" << right << setw(30) << a_results[n].m_description << ": ";
        
        int count = int(a_results[n].m_fitness / big_fit * 50.1);
        
        for (int i = 0; i < count; ++i)
            cout << "*";
        
        cout << right << setw(55 - count) << " ("
             << a_results[n].m_fitness
             << ")";
    }
    
    cout << "\n\nAcovea is done.\n" << endl;
}

// constructor
acovea_world::acovea_world(acovea_listener & a_listener,
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
                           size_t a_generations)
  : m_generations(a_generations),
    m_target(a_target),
    m_listener(a_listener),
    m_input_name(a_bench_name),
    m_mutator(a_mutation_rate, a_target),
    m_reproducer(a_crossover_rate, a_target),
    m_migrator(size_t(a_population_size * a_migration_rate + 0.5)),
    m_null_scaler(),
    m_sigma_scaler(),
    m_selector(size_t(a_population_size * a_survival_rate + 0.5)),
    m_reporter(a_bench_name,a_number_of_populations,a_target,a_listener,a_mode),
    m_evocosm(NULL),
    m_mode(a_mode)
{
    // pick a fitness scaler based on argument
    scaler< acovea_organism > * chosen_scaler;
    string scaler_name;
    string scaler_code;
    
    if (a_use_scaling)
    {
        chosen_scaler = &m_sigma_scaler;
        scaler_name = "sigma";
        scaler_code  = "s";
    }
    else
    {
        chosen_scaler = &m_null_scaler;
        scaler_name = "none";
        scaler_code = "na"; // not any
    }
    
    static const char * MODE_NAME[3] =
    {
        "speed", "size", "return value"
    };
    
    char time_text[100];
    time_t now = time(NULL);
    strftime(time_text,100,"%Y %b %d %X\n",localtime(&now));
    
    char hostname[256];
    gethostname(hostname,256);
    
    // fill list and string versions of version command
    char version_text[4096] = { 0 };

    vector<string> command = m_target.get_get_app_version();
    
    if (command.size() > 0)
    {
        // allocate array of string pointers for exec
        char ** argv = new char * [command.size()];
    
        for (int n = 0; n < command.size(); ++n)
            argv[n] = strdup(command[n].c_str());

        // terminate argument list
        argv[command.size()] = NULL;

        // constants for I/O descriptors
        static const int PIPE_IN  = 0;
        static const int PIPE_OUT = 1;
        static const int PIPE_ERR = 2;

        // create pipe
        int fds[2];
        pipe(fds);

        // fork and exec program to get version
        pid_t child_pid;
        int child_retval;
        child_pid = fork();

        if (child_pid == 0)
        {
            // redirect std. output and error for child
            close(STDOUT_FILENO);
            dup2(fds[PIPE_OUT],STDOUT_FILENO);

            close(fds[PIPE_IN]);
            close(fds[PIPE_OUT]);

            execvp(argv[0],argv);
        }

        // redirect std. input for parent
        close(STDIN_FILENO);
        dup2(fds[PIPE_IN],STDIN_FILENO);

        close(fds[PIPE_IN]);
        close(fds[PIPE_OUT]);

        wait(&child_retval);

        if (child_retval == 0)
            fgets(version_text,4096,stdin);
        else
            strcpy(version_text,"unavailable");
    
        // free memory
        free(argv);
    }
    else
        strcpy(version_text,"not requested");

    // display the header        
    m_config_text << "\n   test application: " << a_bench_name << flush
                  << "\n        test system: " << hostname
                  << "\n config description: " << m_target.get_description()
                  << " (version "              << m_target.get_config_version() << ")"
                  << "\n test configuration: " << m_target.get_config_name()
                  << "\n     acovea version: " << ACOVEA_VERSION
                  << "\n    evocosm version: " << libevocosm::globals::version()
                  << "\napplication version: " << m_target.get_app_name() << " " << version_text
                  << "\n   # of populations: " << a_number_of_populations
                  << "\n    population size: " << a_population_size
                  << "\n      survival rate: " << (a_survival_rate  * 100) << "% (" << size_t(a_population_size * a_survival_rate + 0.5) << ")"
                  << "\n     migration rate: " << (a_migration_rate * 100) << "% (" << size_t(a_population_size * a_migration_rate + 0.5) << ")"
                  << "\n      mutation rate: " << (a_mutation_rate  * 100) << "%"
                  << "\n     crossover rate: " << (a_crossover_rate * 100) << "%"
                  << "\n    fitness scaling: " << scaler_name
                  << "\n generations to run: " << a_generations
                  << "\n random number seed: " << libevocosm::globals::get_seed()
                  << "\n       testing mode: " << MODE_NAME[a_mode]
                  << "\n\n    test start time: " << time_text
                  << "\n" << endl;
           
    m_listener.report_config(m_config_text.str());
    m_reporter.set_config_text(m_config_text.str());

    // create evocosm with requested arguments
    m_evocosm = new evocosm<acovea_organism, acovea_landscape> (m_listener,
                                                                a_population_size,
                                                                a_number_of_populations,
                                                                0,
                                                                1,
                                                                m_mutator,
                                                                m_reproducer,
                                                                *chosen_scaler,
                                                                m_migrator,
                                                                m_selector,
                                                                m_reporter,
                                                                *this,
                                                                *this,
                                                                true);
}

acovea_organism acovea_world::create()
{
    return acovea_organism(m_target);
}

void acovea_world::append(vector<acovea_organism> & a_population, size_t a_size)
{
    // fill remaining population with random values    
    for (size_t i = 0; i < a_size; ++i)
        a_population.push_back(acovea_organism(m_target));
}

acovea_landscape acovea_world::generate()
{
    return acovea_landscape(m_input_name,m_mode,m_target,m_listener);
}

double acovea_world::run()
{
    double fitness = 0.0;

    // continue for specified number of iterations
    for (size_t count = 1; count <= m_generations; ++count)
    {
        // run a generation
        bool keep_going =  m_evocosm->run_generation(count == m_generations,fitness);
        
        if (!keep_going)
        {
            m_listener.report_error("run aborted\n");
            break;
        }
    }
    
    // announce that we're finished
    m_listener.run_complete();
    
    return fitness;
}
            
// terminate run
void acovea_world::terminate()
{
    m_evocosm->terminate();
}

acovea_world::~acovea_world()
{
    delete m_evocosm;
}

