/****************************************************************************************************************/
/*                                                                                                              */
/*   OpenNN: Open Neural Networks Library                                                                       */
/*   www.artelnics.com/opennn                                                                                   */
/*                                                                                                              */
/*   S I M U L A T E D   A N N E A L I N G   O R D E R   C L A S S                                              */
/*                                                                                                              */
/*   Fernando Gomez                                                                                             */
/*   Artelnics - Making intelligent use of data                                                                 */
/*   fernandogomez@artelnics.com                                                                                */
/*                                                                                                              */
/****************************************************************************************************************/


// OpenNN includes

#include "simulated_annealing_order.h"

namespace OpenNN
{

// DEFAULT CONSTRUCTOR

/// Default constructor.

SimulatedAnnealingOrder::SimulatedAnnealingOrder(void)
    : OrderSelectionAlgorithm()
{
    set_default();
}


// TRAINING STRATEGY CONSTRUCTOR

/// Training strategy constructor.
/// @param new_training_strategy_pointer Pointer to a training strategy object.

SimulatedAnnealingOrder::SimulatedAnnealingOrder(TrainingStrategy* new_training_strategy_pointer)
    : OrderSelectionAlgorithm(new_training_strategy_pointer)
{
    set_default();
}


// FILE CONSTRUCTOR

/// File constructor.
/// @param file_name Name of XML simulated annealing order file.

SimulatedAnnealingOrder::SimulatedAnnealingOrder(const std::string& file_name)
    : OrderSelectionAlgorithm(file_name)
{
    load(file_name);
}


// XML CONSTRUCTOR

/// XML constructor.
/// @param simulated_annealing_order_document Pointer to a TinyXML document containing the simulated annealing order data.

SimulatedAnnealingOrder::SimulatedAnnealingOrder(const tinyxml2::XMLDocument& simulated_annealing_order_document)
    : OrderSelectionAlgorithm(simulated_annealing_order_document)
{
    from_XML(simulated_annealing_order_document);
}


// DESTRUCTOR

/// Destructor.

SimulatedAnnealingOrder::~SimulatedAnnealingOrder(void)
{
}

// METHODS

// const double& get_cooling_rate(void) const method

/// Returns the temperature reduction factor for the simulated annealing method.

const double& SimulatedAnnealingOrder::get_cooling_rate(void) const
{
    return(cooling_rate);
}

// const size_t& get_maximum_generalization_failures(void) const method

/// Returns the maximum number of generalization failures in the model order selection algorithm.

const size_t& SimulatedAnnealingOrder::get_maximum_generalization_failures(void) const
{
    return(maximum_generalization_failures);
}

// const double& get_minimum_temperature(void) const method

/// Returns the minimum temperature reached in the simulated annealing model order selection algorithm.

const double& SimulatedAnnealingOrder::get_minimum_temperature(void) const
{
    return(minimum_temperature);
}

// void set_default(void) method 

/// Sets the members of the model selection object to their default values.

void SimulatedAnnealingOrder::set_default(void)
{
    cooling_rate = 0.5;

    maximum_generalization_failures = 3;
    minimum_temperature = 1.0e-3;
}

// void set_cooling_rate(const double&) method

/// Sets the cooling rate for the simulated annealing.
/// @param new_cooling_rate Temperature reduction factor.

void SimulatedAnnealingOrder::set_cooling_rate(const double& new_cooling_rate)
{
#ifdef __OPENNN_DEBUG__

    if(new_cooling_rate <= 0)
    {
        std::ostringstream buffer;
        buffer << "OpenNN Exception: SimulatedAnnealingOrder class.\n"
               << "void set_cooling_rate(const size_t&) method.\n"
               << "Cooling rate must be greater than 0.\n";

        throw std::logic_error(buffer.str());
    }

    if(new_cooling_rate >= 1)
    {
        std::ostringstream buffer;
        buffer << "OpenNN Exception: SimulatedAnnealingOrder class.\n"
               << "void set_cooling_rate(const size_t&) method.\n"
               << "Cooling rate must be less than 1.\n";

        throw std::logic_error(buffer.str());
    }

#endif

    cooling_rate = new_cooling_rate;
}

// void set_maximum_generalization_failures(const size_t&) method

/// Sets the maximum generalization failures for the simulated annealing order selection algorithm.
/// @param new_maximum_performance_failures Maximum number of generalization failures in the simulated annealing order selection algorithm.

void SimulatedAnnealingOrder::set_maximum_generalization_failures(const size_t& new_maximum_performance_failures)
{
#ifdef __OPENNN_DEBUG__

    if (new_maximum_performance_failures <= 0)
    {
        std::ostringstream buffer;

        buffer << "OpenNN Exception: SimulatedAnnealingOrder class.\n"
               << "void set_maximum_generalization_failures(const size_t&) method.\n"
               << "Maximum generalization failures must be greater than 0.\n";

        throw std::logic_error(buffer.str());
    }

#endif

    maximum_generalization_failures = new_maximum_performance_failures;
}

// void set_minimum_temperature(const double&) method

/// Sets the minimum temperature for the simulated annealing order selection algorithm.
/// @param new_minimum_temperature Value of the minimum temperature.

void SimulatedAnnealingOrder::set_minimum_temperature(const double& new_minimum_temperature)
{
#ifdef __OPENNN_DEBUG__

    if (new_minimum_temperature < 0)
    {
        std::ostringstream buffer;

        buffer << "OpenNN Exception: SimulatedAnnealingOrder class.\n"
               << "void set_minimum_temperature(const double&) method.\n"
               << "Minimum temperature must be equal or greater than 0.\n";

        throw std::logic_error(buffer.str());
    }

#endif

    minimum_temperature = new_minimum_temperature;
}

// SimulatedAnnealingOrderResults* perform_order_selection(void) method

/// Perform the order selection with the simulated annealing method.

SimulatedAnnealingOrder::SimulatedAnnealingOrderResults* SimulatedAnnealingOrder::perform_order_selection(void)
{
    SimulatedAnnealingOrderResults* results = new SimulatedAnnealingOrderResults();

    NeuralNetwork* neural_network_pointer = training_strategy_pointer->get_performance_functional_pointer()->get_neural_network_pointer();
    MultilayerPerceptron* multilayer_perceptron_pointer = neural_network_pointer->get_multilayer_perceptron_pointer();

    const size_t inputs_number = multilayer_perceptron_pointer->get_inputs_number();
    const size_t outputs_number = multilayer_perceptron_pointer->get_outputs_number();

    size_t optimal_order, current_order;
    Vector<double> optimum_performance(2);
    Vector<double> current_order_performance(2);
    Vector<double> optimum_parameters, current_parameters;

    Vector<double> history_row(2);
    Vector<double> parameters_history_row;
    double current_training_performance, current_generalization_performance;

    bool end = false;
    size_t iterations = 0;
    size_t generalization_failures = 0;
    size_t random_failures = 0;
    size_t upper_bound;
    size_t lower_bound;

    time_t beginning_time, current_time;
    double elapsed_time;

    double temperature;
    double boltzmann_probability;
    double random_uniform;

    if (display)
        std::cout << "Performing order selection with simulated annealing method..." << std::endl;

    time(&beginning_time);

    optimal_order = (size_t)(minimum_order +
                             calculate_random_uniform(0.,1.)*(maximum_order - minimum_order));
    optimum_performance = calculate_performances(optimal_order);
    optimum_parameters = get_parameters_order(optimal_order);

    current_training_performance = optimum_performance[0];
    current_generalization_performance = optimum_performance[1];

    temperature = current_generalization_performance;

    if (reserve_performance_data)
    {
        history_row[0] = optimal_order;
        history_row[1] = current_training_performance;
        results->performance_data.push_back(history_row);
    }

    if (reserve_generalization_performance_data)
    {
        history_row[0] = optimal_order;
        history_row[1] = current_generalization_performance;
        results->generalization_performance_data.push_back(history_row);
    }

    if (reserve_parameters_data)
    {
        parameters_history_row = get_parameters_order(optimal_order);
        parameters_history_row.insert(parameters_history_row.begin(),optimal_order);
        results->parameters_data.push_back(parameters_history_row);
    }

    time(&current_time);
    elapsed_time = difftime(current_time, beginning_time);

    if (display)
    {
        std::cout << "Initial values : " << std::endl;
        std::cout << "Hidden perceptrons : " << optimal_order << std::endl;
        std::cout << "Final Training Performance : " << optimum_performance[0] << std::endl;
        std::cout << "Final generalization performance : " << optimum_performance[1] << std::endl;
        std::cout << "Temperature : " << temperature << std::endl;
        std::cout << "Elapsed time : " << elapsed_time << std::endl;
    }

    while (!end){

        upper_bound = fmin(maximum_order, optimal_order + (maximum_order-minimum_order)/3);
        if (optimal_order <= (maximum_order-minimum_order)/3)
            lower_bound = minimum_order;
        else
            lower_bound = optimal_order - (maximum_order-minimum_order)/3;

        current_order = (size_t)(lower_bound + calculate_random_uniform(0.,1.)*(upper_bound - lower_bound));
        while (current_order == optimal_order)
        {
            current_order = (size_t)(lower_bound + calculate_random_uniform(0.,1.)*(upper_bound - lower_bound));
            random_failures++;
            if (random_failures >= 5 && optimal_order != minimum_order)
                current_order = optimal_order - 1;
            else if (random_failures >= 5 && optimal_order != maximum_order)
                current_order = optimal_order + 1;
        }
        random_failures = 0;

        current_order_performance = calculate_performances(current_order);
        current_training_performance = current_order_performance[0];
        current_generalization_performance = current_order_performance[1];
        current_parameters = get_parameters_order(optimal_order);

        boltzmann_probability = fmin(1, exp(-(current_generalization_performance-optimum_performance[1])/temperature));
        random_uniform = calculate_random_uniform(0.,1.);

        if ((boltzmann_probability <= random_uniform)
            || (fabs(optimum_performance[1]-current_generalization_performance) <= tolerance
                && current_order >= optimal_order))
            // Generalization failures
        {
            generalization_failures++;
        }else
            // Generalization success
        {
            optimal_order = current_order;
            optimum_performance = current_order_performance;
            optimum_parameters = get_parameters_order(optimal_order);
        }

        time(&current_time);
        elapsed_time = difftime(current_time, beginning_time);

        if (reserve_performance_data)
        {
            history_row[0] = current_order;
            history_row[1] = current_order_performance[0];
            results->performance_data.push_back(history_row);
        }

        if (reserve_generalization_performance_data)
        {
            history_row[0] = current_order;
            history_row[1] = current_order_performance[1];
            results->generalization_performance_data.push_back(history_row);
        }

        if (reserve_parameters_data)
        {
            parameters_history_row = get_parameters_order(current_order);
            parameters_history_row.insert(parameters_history_row.begin(),current_order);
            results->parameters_data.push_back(parameters_history_row);
        }

        temperature = cooling_rate*temperature;

        iterations++;

        // Stopping criteria

        if (temperature < minimum_temperature)
        {
            end = true;
            if (display)
                std::cout << "Minimum temperature reached." << std::endl;
            results->stopping_condition = SimulatedAnnealingOrder::MinimumTemperature;
        }else if (elapsed_time > maximum_time)
        {
            end = true;
            if (display)
                std::cout << "Maximum time reached." << std::endl;
            results->stopping_condition = SimulatedAnnealingOrder::MaximumTime;
        }else if (optimum_performance[1] < generalization_performance_goal)
        {
            end = true;
            if (display)
                std::cout << "Generalization performance reached." << std::endl;
            results->stopping_condition = SimulatedAnnealingOrder::GeneralizationPerformanceGoal;
        }else if (generalization_failures >= maximum_generalization_failures)
        {
            end = true;
            if (display)
                std::cout << "Maximum generalization performance failures("<<generalization_failures<<") reached." << std::endl;
            results->stopping_condition = SimulatedAnnealingOrder::MaximumGeneralizationFailures;
        }else if (iterations >= maximum_iterations_number)
        {
            end = true;
            if (display)
                std::cout << "Maximum number of iterations reached." << std::endl;
            results->stopping_condition = SimulatedAnnealingOrder::MaximumIterations;
        }

        if (display)
        {
            std::cout << "Iteration : " << iterations << std::endl;
            std::cout << "Hidden perceptron number : " << optimal_order << std::endl;
            std::cout << "Final Training Performance : " << optimum_performance[0] << std::endl;
            std::cout << "Final generalization performance : " << optimum_performance[1] << std::endl;
            std::cout << "Current temperature : " << temperature << std::endl;
            std::cout << "Elapsed time : " << elapsed_time << std::endl;
        }

    }

    if (display)
        std::cout << "Optimal order : " << optimal_order << std:: endl;

    multilayer_perceptron_pointer->set(inputs_number, optimal_order, outputs_number);
    multilayer_perceptron_pointer->set_parameters(optimum_parameters);

    if (reserve_minimal_parameters)
        results->minimal_parameters = optimum_parameters;

    results->optimal_order = optimal_order;
    results->final_performance = optimum_performance[0];
    results->final_generalization_performance = optimum_performance[1];
    results->elapsed_time = elapsed_time;
    results->iterations_number = iterations;

    return(results);
}

// tinyxml2::XMLDocument* to_XML(void) const method

/// Prints to the screen the simulated annealing order parameters, the stopping criteria
/// and other user stuff concerning the simulated annealing order object.

tinyxml2::XMLDocument* SimulatedAnnealingOrder::to_XML(void) const
{
   std::ostringstream buffer;

   tinyxml2::XMLDocument* document = new tinyxml2::XMLDocument;

   // Order Selection algorithm

   tinyxml2::XMLElement* root_element = document->NewElement("SimulatedAnnealingOrder");

   document->InsertFirstChild(root_element);

   tinyxml2::XMLElement* element = NULL;
   tinyxml2::XMLText* text = NULL;

   // Minimum order
   {
   element = document->NewElement("MinimumOrder");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << minimum_order;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Maximum order
   {
   element = document->NewElement("MaximumOrder");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << maximum_order;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Parameters assays number
   {
   element = document->NewElement("TrialsNumber");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << trials_number;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Performance calculation method
   {
   element = document->NewElement("PerformanceCalculationMethod");
   root_element->LinkEndChild(element);

   text = document->NewText(write_performance_calculation_method().c_str());
   element->LinkEndChild(text);
   }

   // Cooling rate
   {
   element = document->NewElement("CoolingRate");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << cooling_rate;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Reserve parameters data
   {
   element = document->NewElement("ReserveParametersData");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << reserve_parameters_data;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Reserve performance data
   {
   element = document->NewElement("ReservePerformanceData");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << reserve_performance_data;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Reserve generalization performance data
   {
   element = document->NewElement("ReserveGeneralizationPerformanceData");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << reserve_generalization_performance_data;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Reserve minimal parameters
   {
   element = document->NewElement("ReserveMinimalParameters");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << reserve_minimal_parameters;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Display
   {
   element = document->NewElement("Display");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << display;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Generalization performance goal
   {
   element = document->NewElement("GeneralizationPerformanceGoal");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << generalization_performance_goal;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Maximum iterations
   {
   element = document->NewElement("MaximumIterationsNumber");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << maximum_iterations_number;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Maximum time
   {
   element = document->NewElement("MaximumTime");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << maximum_time;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Tolerance
   {
   element = document->NewElement("Tolerance");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << tolerance;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Maximum generalization failures
   {
   element = document->NewElement("MaximumGeneralizationFailures");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << maximum_generalization_failures;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   // Minimum temperature
   {
   element = document->NewElement("MinimumTemperature");
   root_element->LinkEndChild(element);

   buffer.str("");
   buffer << minimum_temperature;

   text = document->NewText(buffer.str().c_str());
   element->LinkEndChild(text);
   }

   return(document);
}

// void from_XML(const tinyxml2::XMLDocument&) method

/// Deserializes a TinyXML document into this simulated annealing order object.
/// @param document TinyXML document containing the member data.

void SimulatedAnnealingOrder::from_XML(const tinyxml2::XMLDocument& document)
{
    const tinyxml2::XMLElement* root_element = document.FirstChildElement("SimulatedAnnealingOrder");

    if(!root_element)
    {
        std::ostringstream buffer;

        buffer << "OpenNN Exception: IncrementalOrder class.\n"
               << "void from_XML(const tinyxml2::XMLDocument&) method.\n"
               << "SimulatedAnnealingOrder element is NULL.\n";

        throw std::logic_error(buffer.str());
    }

    // Minimum order
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("MinimumOrder");

        if(element)
        {
           const size_t new_minimum_order = atoi(element->GetText());

           try
           {
              minimum_order = new_minimum_order;
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Maximum order
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("MaximumOrder");

        if(element)
        {
           const size_t new_maximum_order = atoi(element->GetText());

           try
           {
              maximum_order = new_maximum_order;
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Parameters assays number
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("TrialsNumber");

        if(element)
        {
           const size_t new_trials_number = atoi(element->GetText());

           try
           {
              set_trials_number(new_trials_number);
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Performance calculation method
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("PerformanceCalculationMethod");

        if(element)
        {
           const std::string new_performance_calculation_method = element->GetText();

           try
           {
              set_performance_calculation_method(new_performance_calculation_method);
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Cooling rate
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("CoolingRate");

        if(element)
        {
           const double new_cooling_rate = atof(element->GetText());

           try
           {
              set_cooling_rate(new_cooling_rate);
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Reserve parameters data
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("ReserveParametersData");

        if(element)
        {
           const std::string new_reserve_parameters_data = element->GetText();

           try
           {
              set_reserve_parameters_data(new_reserve_parameters_data != "0");
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Reserve performance data
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("ReservePerformanceData");

        if(element)
        {
           const std::string new_reserve_performance_data = element->GetText();

           try
           {
              set_reserve_performance_data(new_reserve_performance_data != "0");
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Reserve generalization performance data
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("ReserveGeneralizationPerformanceData");

        if(element)
        {
           const std::string new_reserve_generalization_performance_data = element->GetText();

           try
           {
              set_reserve_generalization_performance_data(new_reserve_generalization_performance_data != "0");
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Reserve minimal parameters
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("ReserveMinimalParameters");

        if(element)
        {
           const std::string new_reserve_minimal_parameters = element->GetText();

           try
           {
              set_reserve_minimal_parameters(new_reserve_minimal_parameters != "0");
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Display
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("Display");

        if(element)
        {
           const std::string new_display = element->GetText();

           try
           {
              set_display(new_display != "0");
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Generalization performance goal
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("GeneralizationPerformanceGoal");

        if(element)
        {
           const double new_generalization_performance_goal = atof(element->GetText());

           try
           {
              set_generalization_performance_goal(new_generalization_performance_goal);
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Maximum iterations number
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("MaximumIterationsNumber");

        if(element)
        {
           const double new_maximum_iterations_number = atoi(element->GetText());

           try
           {
              set_maximum_iterations_number(new_maximum_iterations_number);
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Maximum time
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("MaximumTime");

        if(element)
        {
           const double new_maximum_time = atoi(element->GetText());

           try
           {
              set_maximum_time(new_maximum_time);
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Tolerance
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("Tolerance");

        if(element)
        {
           const double new_tolerance = atof(element->GetText());

           try
           {
              set_tolerance(new_tolerance);
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Maximum generalization failures
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("MaximumGeneralizationFailures");

        if(element)
        {
           const double new_maximum_generalization_failures = atoi(element->GetText());

           try
           {
              set_maximum_generalization_failures(new_maximum_generalization_failures);
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }

    // Minimum temperature
    {
        const tinyxml2::XMLElement* element = root_element->FirstChildElement("MinimumTemperature");

        if(element)
        {
           const double new_minimum_temperature = atof(element->GetText());

           try
           {
              set_minimum_temperature(new_minimum_temperature);
           }
           catch(const std::logic_error& e)
           {
              std::cout << e.what() << std::endl;
           }
        }
    }
}

// void save(const std::string&) const method

/// Saves to a XML-type file the members of the simulated annealing order object.
/// @param file_name Name of simulated annealing order XML-type file.

void SimulatedAnnealingOrder::save(const std::string& file_name) const
{
   tinyxml2::XMLDocument* document = to_XML();

   document->SaveFile(file_name.c_str());

   delete document;
}


// void load(const std::string&) method

/// Loads a simulated annealing order object from a XML-type file.
/// @param file_name Name of simulated annealing order XML-type file.

void SimulatedAnnealingOrder::load(const std::string& file_name)
{
   set_default();

   tinyxml2::XMLDocument document;

   if (document.LoadFile(file_name.c_str()))
   {
      std::ostringstream buffer;

      buffer << "OpenNN Exception: SimulatedAnnealingOrder class.\n"
             << "void load(const std::string&) method.\n"
             << "Cannot load XML file " << file_name << ".\n";

      throw std::logic_error(buffer.str());
   }

   from_XML(document);
}

}
