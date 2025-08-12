#include "ParameterStorage.hpp"

/**
 * \file ParameterStorage.cpp
 * \ingroup lcc
 */

ParameterStorage::ParameterStorage(std::string _filename, int precision) :
    PRECISION(precision),
    filename(_filename)
{
    loadFile();
}

void ParameterStorage::register_on_param_changed_callback(std::function<void(std::string)> _on_param_changed_callback) {
    on_param_changed_callback = _on_param_changed_callback;
}

void ParameterStorage::loadFile() {
    loadFile(filename);
}
void ParameterStorage::loadFile(std::string _filename) {
    filename = _filename;

    std::unique_lock<std::mutex> lock(param_storage_mutex);
    param_storage.clear();
    lock.unlock();

    YAML::Node parsedFile = YAML::LoadFile(_filename);

    YAML::Node params = parsedFile["parameters"];
    YAML::Node params_bool = params["bool"];
    YAML::Node params_uint64_t = params["uint64_t"];
    YAML::Node params_int = params["int"];
    YAML::Node params_double = params["double"];
    YAML::Node params_string = params["string"];
    YAML::Node params_ints = params["ints"];
    YAML::Node params_doubles = params["doubles"];
    
    if (!(params_bool.IsMap() 
        && params_uint64_t.IsMap() 
        && params_int.IsMap() 
        && params_double.IsMap() 
        && params_string.IsMap() 
        && params_ints.IsMap() 
        && params_doubles.IsMap())) 
    {
        throw std::domain_error("The input file is not conformant with the specification - all types must be stored in maps");
    }

    for (YAML::const_iterator it=params_bool.begin();it!=params_bool.end();++it) {
        set_parameter_bool(it->first.as<std::string>(), it->second["value"].as<bool>(), it->second["info"].as<std::string>());
    }
    for (YAML::const_iterator it=params_uint64_t.begin();it!=params_uint64_t.end();++it) {
        set_parameter_uint64_t(it->first.as<std::string>(), it->second["value"].as<uint64_t>(), it->second["info"].as<std::string>());
    }
    for (YAML::const_iterator it=params_int.begin();it!=params_int.end();++it) {
        set_parameter_int(it->first.as<std::string>(), it->second["value"].as<int32_t>(), it->second["info"].as<std::string>());
    }
    for (YAML::const_iterator it=params_double.begin();it!=params_double.end();++it) {
        set_parameter_double(it->first.as<std::string>(), it->second["value"].as<double>(), it->second["info"].as<std::string>());
    }
    for (YAML::const_iterator it=params_string.begin();it!=params_string.end();++it) {
        set_parameter_string(it->first.as<std::string>(), it->second["value"].as<std::string>(), it->second["info"].as<std::string>());
    }
    for (YAML::const_iterator outer_it=params_ints.begin();outer_it!=params_ints.end();++outer_it) {
        std::vector<int32_t> ints;

        if (!outer_it->second["value"].IsSequence()) {
            //Is this really a good idea, or would logging &ignoring the paramter be better in this case?
            throw std::domain_error("The input file is not conformant with the specification - ints must contain sequences");
        }

        for (YAML::const_iterator inner_it=outer_it->second["value"].begin();inner_it!=outer_it->second["value"].end();++inner_it) {
            ints.push_back(inner_it->as<int32_t>());
        }
        set_parameter_ints(outer_it->first.as<std::string>(), ints, outer_it->second["info"].as<std::string>());
        // std::cout << "Loaded " << outer_it->first.as<std::string>() << " with values " << std::endl;
        // for (auto val : ints) {
        //     std::cout << "\t" << val << std::endl;
        // }
    }
    for (YAML::const_iterator outer_it=params_doubles.begin();outer_it!=params_doubles.end();++outer_it) {
        std::vector<double> doubles;

        if (!outer_it->second["value"].IsSequence()) {
            //Is this really a good idea, or would logging &ignoring the paramter be better in this case?
            throw std::domain_error("The input file is not conformant with the specification - doubles must contain sequences");
        }

        for (YAML::const_iterator inner_it=outer_it->second["value"].begin();inner_it!=outer_it->second["value"].end();++inner_it) {
            doubles.push_back(inner_it->as<double>());
        }
        set_parameter_doubles(outer_it->first.as<std::string>(), doubles, outer_it->second["info"].as<std::string>());
    }
}

int ParameterStorage::get_precision() {
    return PRECISION;
}

void ParameterStorage::storeFile() {
    storeFile(filename);
}
void ParameterStorage::storeFile(std::string _filename) {
    filename = _filename;
    
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "parameters";
    out << YAML::Value << YAML::BeginMap;
    
    out << YAML::Key << "bool";
    out << YAML::Value << YAML::BeginMap;
    for (auto const& key : list_bool()) {
        ParameterWithDescription param;
        get_parameter(key, param);
        out << YAML::Key << key;
        out << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "value";
        out << YAML::Value << param.parameter_data.value_bool();
        out << YAML::Key << "info";
        out << YAML::Value << YAML::DoubleQuoted << param.parameter_description;
        out << YAML::EndMap;
    }
    out << YAML::EndMap;

    out << YAML::Key << "uint64_t";
    out << YAML::Value << YAML::BeginMap;
    for (auto const& key : list_uint64_t()) {
        ParameterWithDescription param;
        get_parameter(key, param);
        out << YAML::Key << key;
        out << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "value";
        out << YAML::Value << param.parameter_data.value_uint64_t();
        out << YAML::Key << "info";
        out << YAML::Value << YAML::DoubleQuoted << param.parameter_description;
        out << YAML::EndMap;
    }
    out << YAML::EndMap;

    out << YAML::Key << "int";
    out << YAML::Value << YAML::BeginMap;
    for (auto const& key : list_int()) {
        ParameterWithDescription param;
        get_parameter(key, param);
        out << YAML::Key << key;
        out << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "value";
        out << YAML::Value << param.parameter_data.values_int32().at(0);
        out << YAML::Key << "info";
        out << YAML::Value << YAML::DoubleQuoted << param.parameter_description;
        out << YAML::EndMap;
    }
    out << YAML::EndMap;

    out << YAML::Key << "double";
    out << YAML::Value << YAML::BeginMap;
    for (auto const& key : list_double()) {
        ParameterWithDescription param;
        get_parameter(key, param);
        out << YAML::Key << key;
        out << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "value";
        out << YAML::Value << YAML::DoublePrecision(PRECISION) << param.parameter_data.values_double().at(0);
        out << YAML::Key << "info";
        out << YAML::Value << YAML::DoubleQuoted << param.parameter_description;
        out << YAML::EndMap;
    }
    out << YAML::EndMap;

    out << YAML::Key << "string";
    out << YAML::Value << YAML::BeginMap;
    for (auto const& key : list_string()) {
        ParameterWithDescription param;
        get_parameter(key, param);
        out << YAML::Key << key;
        out << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "value";
        out << YAML::Value << YAML::DoubleQuoted << param.parameter_data.value_string();
        out << YAML::Key << "info";
        out << YAML::Value << YAML::DoubleQuoted << param.parameter_description;
        out << YAML::EndMap;
    }
    out << YAML::EndMap;

    out << YAML::Key << "ints";
    out << YAML::Value << YAML::BeginMap;
    for (auto const& key : list_ints()) {
        ParameterWithDescription param;
        get_parameter(key, param);
        out << YAML::Key << key;
        out << YAML::Value << YAML::BeginMap;

        std::vector<int32_t> value;
        for (int32_t val : param.parameter_data.values_int32()) {
            value.push_back(val);
        }

        out << YAML::Key << "value";
        out << YAML::Value << value;
        out << YAML::Key << "info";
        out << YAML::Value << YAML::DoubleQuoted << param.parameter_description;
        out << YAML::EndMap;
    }
    out << YAML::EndMap;
    
    out << YAML::Key << "doubles";
    out << YAML::Value << YAML::BeginMap;
    for (auto const& key : list_doubles()) {
        ParameterWithDescription param;
        get_parameter(key, param);
        out << YAML::Key << key;
        out << YAML::Value << YAML::BeginMap;

        std::vector<double> value;
        for (double val : param.parameter_data.values_double()) {
            value.push_back(val);
        }

        out << YAML::Key << "value";
        out << YAML::Value << YAML::DoublePrecision(PRECISION) << value;
        out << YAML::Key << "info";
        out << YAML::Value << YAML::DoubleQuoted << param.parameter_description;
        out << YAML::EndMap;
    }
    out << YAML::EndMap;

    out << YAML::EndMap << YAML::EndMap;

    std::ofstream fileStream;
    fileStream.open(_filename, std::ofstream::out | std::ofstream::trunc);
    fileStream << out.c_str() << std::endl;
    fileStream.close();
}

void ParameterStorage::set_parameter(std::string name, ParameterWithDescription param) {
    std::unique_lock<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        param_storage[name] = param;
    }
    else {
        param_storage.emplace(name, param);
    }
    u_lock.unlock();

    //Call the user of the storage s.t. it can react to the change
    if (on_param_changed_callback) {
        on_param_changed_callback(name);
    }
}

void ParameterStorage::set_parameter_bool(std::string name, bool value, std::string info) {
    //Create parameter object
    ParameterWithDescription param;
    param.parameter_description = info;
    param.parameter_data.name(name);
    param.parameter_data.type(ParameterType::Bool);
    param.parameter_data.value_bool(value);

    //Store the object
    set_parameter(name, param);
}
void ParameterStorage::set_parameter_uint64_t(std::string name, uint64_t value, std::string info) {
    //Create parameter object
    ParameterWithDescription param;
    param.parameter_description = info;
    param.parameter_data.name(name);
    param.parameter_data.type(ParameterType::UInt64);
    param.parameter_data.value_uint64_t(value);

    //Store the object
    set_parameter(name, param);
}
void ParameterStorage::set_parameter_int(std::string name, int32_t value, std::string info) {
    std::vector<int32_t> stdInts;
    stdInts.push_back(value);
    rti::core::vector<int32_t> ints(stdInts);

    //Create parameter object
    ParameterWithDescription param;
    param.parameter_description = info;
    param.parameter_data.name(name);
    param.parameter_data.type(ParameterType::Int32);
    param.parameter_data.values_int32(ints);

    //Store the object
    set_parameter(name, param);
}
void ParameterStorage::set_parameter_double(std::string name, double value, std::string info) {
    std::vector<double> stdDoubles;
    stdDoubles.push_back(value);
    rti::core::vector<double> doubles(stdDoubles);

    //Create parameter object
    ParameterWithDescription param;
    param.parameter_description = info;
    param.parameter_data.name(name);
    param.parameter_data.type(ParameterType::Double);
    param.parameter_data.values_double(doubles);

    //Store the object
    set_parameter(name, param);
}
void ParameterStorage::set_parameter_string(std::string name, std::string value, std::string info) {
    //Create parameter object
    ParameterWithDescription param;
    param.parameter_description = info;
    param.parameter_data.name(name);
    param.parameter_data.type(ParameterType::String);
    param.parameter_data.value_string(value);

    //Store the object
    set_parameter(name, param);
}
void ParameterStorage::set_parameter_string(std::string name, const char* value, std::string info) {
    //Create parameter object
    ParameterWithDescription param;
    param.parameter_description = info;
    param.parameter_data.name(name);
    param.parameter_data.type(ParameterType::String);
    param.parameter_data.value_string(value);

    //Store the object
    set_parameter(name, param);
}
void ParameterStorage::set_parameter_ints(std::string name, std::vector<int32_t> value, std::string info) {
    //Create parameter object
    ParameterWithDescription param;
    param.parameter_description = info;
    param.parameter_data.name(name);
    param.parameter_data.type(ParameterType::Vector_Int32);
    param.parameter_data.values_int32(value);

    //Store the object
    set_parameter(name, param);
}
void ParameterStorage::set_parameter_doubles(std::string name, std::vector<double> value, std::string info) {
    //Create parameter object
    ParameterWithDescription param;
    param.parameter_description = info;
    param.parameter_data.name(name);
    param.parameter_data.type(ParameterType::Vector_Double);
    param.parameter_data.values_double(value);

    //Store the object
    set_parameter(name, param);
}

bool ParameterStorage::get_parameter_bool(std::string name, bool& value) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        if ((param_storage[name]).parameter_data.type() == ParameterType::Bool) {
            value = (param_storage[name]).parameter_data.value_bool();
            return true;
        }
    }
    return false;
}
bool ParameterStorage::get_parameter_uint64_t(std::string name, uint64_t& value) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        if ((param_storage[name]).parameter_data.type() == ParameterType::UInt64) {
            value = (param_storage[name]).parameter_data.value_uint64_t();
            return true;
        }
    }
    return false;
}
bool ParameterStorage::get_parameter_int(std::string name, int32_t& value) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        if ((param_storage[name]).parameter_data.type() == ParameterType::Int32) {
            value = (param_storage[name]).parameter_data.values_int32().at(0);
            return true;
        }
    }
    return false;
}
bool ParameterStorage::get_parameter_double(std::string name, double& value) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        if ((param_storage[name]).parameter_data.type() == ParameterType::Double) {
            value = (param_storage[name]).parameter_data.values_double().at(0);
            return true;
        }
    }
    return false;
}
bool ParameterStorage::get_parameter_string(std::string name, std::string& value) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        if ((param_storage[name]).parameter_data.type() == ParameterType::String) {
            value = (param_storage[name]).parameter_data.value_string();
            return true;
        }
    }
    return false;
}
bool ParameterStorage::get_parameter_ints(std::string name, std::vector<int32_t>& value) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        if ((param_storage[name]).parameter_data.type() == ParameterType::Vector_Int32) {
            rti::core::vector<int32_t>& rti_vector = (param_storage[name]).parameter_data.values_int32();
            value.clear();
            for (int32_t val : rti_vector) {
                value.push_back(val);
            }
            return true;
        }
    }
    return false;
}
bool ParameterStorage::get_parameter_doubles(std::string name, std::vector<double>& value) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        if ((param_storage[name]).parameter_data.type() == ParameterType::Vector_Double) {
            rti::core::vector<double>& rti_vector = (param_storage[name]).parameter_data.values_double();
            value.clear();
            for (double val : rti_vector) {
                value.push_back(val);
            }
            return true;
        }
    }
    return false;
}

bool ParameterStorage::get_parameter(std::string name, ParameterWithDescription& param) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        //Copy parameter
        param.parameter_data.name(param_storage[name].parameter_data.name());
        param.parameter_data.type(param_storage[name].parameter_data.type());
        param.parameter_data.value_bool(param_storage[name].parameter_data.value_bool());
        param.parameter_data.value_uint64_t(param_storage[name].parameter_data.value_uint64_t());
        param.parameter_data.value_string(param_storage[name].parameter_data.value_string());
        param.parameter_data.values_int32(param_storage[name].parameter_data.values_int32());
        param.parameter_data.values_double(param_storage[name].parameter_data.values_double());
        param.parameter_description = param_storage[name].parameter_description;

        return true;
    }
    return false;
}

void ParameterStorage::delete_parameter(std::string name) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    if (param_storage.find(name) != param_storage.end()) {
        param_storage.erase(name);
    }
}

std::vector<std::string> ParameterStorage::list_bool() {
    return list_names(ParameterType::Bool);
}
std::vector<std::string> ParameterStorage::list_uint64_t() {
    return list_names(ParameterType::UInt64);
}
std::vector<std::string> ParameterStorage::list_int() {
    return list_names(ParameterType::Int32);
}
std::vector<std::string> ParameterStorage::list_double() {
    return list_names(ParameterType::Double);
}
std::vector<std::string> ParameterStorage::list_string() {
    return list_names(ParameterType::String);
}
std::vector<std::string> ParameterStorage::list_ints() {
    return list_names(ParameterType::Vector_Int32);
}
std::vector<std::string> ParameterStorage::list_doubles() {
    return list_names(ParameterType::Vector_Double);
}

std::vector<std::string> ParameterStorage::list_names(ParameterType type) {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    std::vector<std::string> param_names;
    for (auto const& entry : param_storage) {
        if (entry.second.parameter_data.type() == type) {
            param_names.push_back(entry.first);
        }
    }
    return param_names;
}

std::vector<ParameterWithDescription> ParameterStorage::get_all_parameters() {
    std::lock_guard<std::mutex> u_lock(param_storage_mutex);
    std::vector<ParameterWithDescription> params;
    for (auto const& entry : param_storage) {
        params.push_back(entry.second);
    }
    return params;
}