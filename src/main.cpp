#include <string>
#include <vector>
#include <iterator> 
#include <algorithm>

#include <sstream>

#include <iostream>
#include <windows.h>
#include <atlstr.h>

const static char COMMA_DELIM = ',';
const static char START_SECTION_DELIM = '[';
const static char END_SECTION_DELIM = ']';
const static char DOUBLE_QUOTE_DELIM = '\"';
const static char DOT_DELIM = '.';

const static long PRECISION_DOUBLE_REPR = 15;

// убирает разделители в начале строки
inline bool trim_delimeter_on_start(std::string& double_str_reps)
{
    try
    {
        if (double_str_reps[0] == COMMA_DELIM)
        {
            for (size_t k = 0; k < double_str_reps.size(); k++)
            {
                if (double_str_reps[0] != COMMA_DELIM)
                    break;
                else if (double_str_reps[0] == COMMA_DELIM)
                    double_str_reps.erase(0, 1);
            }
        }
    }
    catch(...)
    {
        return false;
    }

    return true;
}

struct sectionValues
{
    std::string section_name;
    std::string section_value;

    bool is_simple_or_nested;
    std::vector<double> section_values;

    sectionValues()
    {
        is_simple_or_nested = false;
    }
};

struct SHPConfigPrj
{
    std::vector<std::pair<std::string, sectionValues>> data;
    std::string initial_config;

    // обновляет значение для переданной секции
    bool update_section_value_by_section_name(const std::string& section_name, const std::string& val_for_replace)
    {
        bool is_success = false;

        if (section_name.size() == 0 || val_for_replace.size() == 0)
            return is_success;

        const std::size_t section_name_idx = initial_config.find(section_name);
        if (section_name_idx == std::string::npos)
            return is_success;

        std::string value_to_replace;
        for (std::pair<std::string, sectionValues>& section_info : data)
        {
            if (section_info.second.section_name == section_name)
            {
                value_to_replace = section_info.second.section_value;
                section_info.second.section_value = val_for_replace;
                break;
            }
        }

        if (value_to_replace.size() == 0)
            return is_success;

        const size_t section_pos = initial_config.find(value_to_replace);
        const size_t section_value_to_replace_pos = initial_config.find(value_to_replace);

        if (section_value_to_replace_pos != std::string::npos)
        {
            is_success = true;
            initial_config.replace(section_pos, value_to_replace.length(), val_for_replace);
        }

        return is_success;
    }

    // считывает индексы подстрок
    void parse_all_value_entries(std::vector<size_t>& vec, std::string data, std::string toSearch)
    {
        size_t pos = data.find(toSearch);
        while (pos != std::string::npos)
        {
            vec.push_back(pos);
            pos = data.find(toSearch, pos + toSearch.size());
        }
    }

    // в методе происходит замена значений списка параметров исходной секции на переданный
    bool replace_section_value_by_index(const std::string& _double_str_reps, const std::vector<double>& values, const size_t idx)
    {
        bool is_success = false;
        // если строка не преобразуется - создастся исключение, и подстрока будет пропущена и last_delim_pos обновится
        const double parsed_test_double = std::stod(_double_str_reps);

        std::ostringstream ss_double_repr;
        ss_double_repr.precision(PRECISION_DOUBLE_REPR);
        ss_double_repr << values[idx];
        std::string value_to_replace = ss_double_repr.str();

        std::vector<size_t> substr_indexes;
        // де факто - проверка на уникальность вхождения, 
        // могут быть случаи когда новое значение для записи - есть как подстрока одного из списка значений секции
        // первое значение всегда ищется в исходной строке, далее уже в обновленной
        parse_all_value_entries(substr_indexes, initial_config, _double_str_reps);

        size_t start_pos_to_replace = 0;
        if (values.size() == 1 && substr_indexes.size() >= 1)
            start_pos_to_replace = substr_indexes[0];
        else
            start_pos_to_replace = substr_indexes.back();

        const size_t section_pos = initial_config.find(_double_str_reps);

        if (section_pos != std::string::npos)
            initial_config.erase(start_pos_to_replace, _double_str_reps.length());
        else
            return is_success;

        initial_config.insert(start_pos_to_replace, value_to_replace);

        is_success = true;
        return is_success;
    }

    // обновляет список значений секции, секция должна быть уникальной без дубликатов
    // если секций несколько, то значение будет заменено для первой
    // размерность переданного вектора значений и текущее количество значений секции должна быть одинаковой
    bool update_values_by_section_name(const std::string& section_name, const std::vector<double>& values)
    {
        bool is_success = false;

        if (section_name.size() == 0 || values.size() == 0)
            return is_success;

        const std::size_t section_name_idx = initial_config.find(section_name);
        if (section_name_idx == std::string::npos)
            return is_success;

        std::vector<double> section_values;
        std::string section_param;
        for (const std::pair<std::string, sectionValues>& section_info : data)
        {
            if (section_info.second.section_name == section_name)
            {
                section_values = section_info.second.section_values;
                section_param = section_info.second.section_value;
                break;
            }
        }

        if(section_values.size() != values.size())
            return is_success;

        section_values.clear();
        std::copy(values.begin(), values.end(), back_inserter(section_values));

        const size_t section_values_end_pos = initial_config.find(']', section_name_idx + section_name.size() - 1);
        const size_t start_slice_idx = ((section_name_idx + section_name.size() + 1) + section_param.size() + 1) + 1;
        const size_t slice_chars_count = section_values_end_pos - start_slice_idx;

        if (section_values_end_pos == std::string::npos || start_slice_idx == std::string::npos || slice_chars_count == 0)
            return is_success;

        // формируем список значений с разделителем ',' в виде строки
        std::string initial_bind_str = initial_config.substr(start_slice_idx, slice_chars_count);
        initial_bind_str += ',';

        // для подстроки (значения) в текущем конфиге, ищем индексы и заменяем текущий срез для последнего найденного индекса искомой подстроки 
        size_t values_replaced_count = 0;
        size_t last_delim_pos = 0;
        for (size_t l = 0; l < initial_bind_str.size(); l++)
        {
            if (initial_bind_str[l] == ',' && l > 0)
            {
                try
                {
                    std::string _double_str_reps = initial_bind_str.substr(last_delim_pos, l - last_delim_pos);

                    if (!trim_delimeter_on_start(_double_str_reps) || _double_str_reps.size() == 0)
                        continue;

                    replace_section_value_by_index(_double_str_reps, values, values_replaced_count);

                    values_replaced_count += 1;
                }
                catch (...)
                {
                    continue;
                }

                last_delim_pos = l;
            }
        }

        is_success = true;
        return is_success;
    }

    // обновляет конкретное значение из списка значений секции по переданному индексу
    // индекс должен начинаться с 0
    bool update_section_value_by_index(const std::string& section_name, const double& value, const size_t index)
    {
        bool is_success = false;

        if (section_name.size() == 0)
            return is_success;

        const std::size_t section_name_idx = initial_config.find(section_name);
        if (section_name_idx == std::string::npos)
            return is_success;

        std::vector<double> section_values;
        std::string section_param;
        for (const std::pair<std::string, sectionValues>& section_info : data)
        {
            if (section_info.second.section_name == section_name)
            {
                section_values = section_info.second.section_values;
                section_param = section_info.second.section_value;
                break;
            }
        }

        if (index > section_values.size())
            return is_success;

        section_values[index] = value;

        const size_t section_values_end_pos = initial_config.find(']', section_name_idx + section_name.size() - 1);
        const size_t start_slice_idx = ((section_name_idx + section_name.size() + 1) + section_param.size() + 1) + 1;
        const size_t slice_chars_count = section_values_end_pos - start_slice_idx;

        if (section_values_end_pos == std::string::npos || start_slice_idx == std::string::npos || slice_chars_count == 0)
            return is_success;

        // формируем список значений с разделителем ',' в виде строки
        std::string initial_bind_str = initial_config.substr(start_slice_idx, slice_chars_count);
        initial_bind_str += ',';

        // для подстроки (значения) в текущем конфиге, ищем индексы и заменяем текущий срез для последнего найденного индекса искомой подстроки 
        size_t values_replaced_count = 0;
        size_t last_delim_pos = 0;

        std::vector<double> values;
        values.push_back(value);
        for (size_t l = 0; l < initial_bind_str.size(); l++)
        {
            if (initial_bind_str[l] == ',' && l > 0)
            {
                try
                {
                    std::string _double_str_reps = initial_bind_str.substr(last_delim_pos, l - last_delim_pos);

                    if (!trim_delimeter_on_start(_double_str_reps) || _double_str_reps.size() == 0)
                        continue;

                    if (index == values_replaced_count)
                    {
                        replace_section_value_by_index(_double_str_reps, values, 0);
                        break;
                    }

                    values_replaced_count += 1;
                }
                catch (...)
                {
                    continue;
                }

                last_delim_pos = l;
            }
        }

        is_success = true;
        return is_success;
    }
};

// проверка на то, что начало из начала строки можно считать значение секции
inline bool is_starting_section_value(const std::string& actual_substr)
{
    if (actual_substr.size() == 0)
        return false;

    const size_t sequence_delim_pos = actual_substr.find(',');
    const size_t sequence_start_section_pos = actual_substr.find('[');

    return sequence_delim_pos < sequence_start_section_pos&& sequence_delim_pos != std::string::npos && sequence_start_section_pos != std::string::npos;
}

// заполнение значений секции
inline void parse_values(const std::string& section_values, std::vector<double>& values)
{
    size_t last_delim_pos = 0;
    for (size_t l = 0; l < section_values.size(); l++)
    {
        if (section_values[l] == ',' && l > 0)
        {
            try
            {
                std::string _double_str_reps = section_values.substr(last_delim_pos, l - last_delim_pos);

                if (!trim_delimeter_on_start(_double_str_reps) || _double_str_reps.size() == 0)
                    continue;

                if (_double_str_reps.size() == 0)
                    continue;

                const double parsed_double = std::stod(_double_str_reps);
                values.push_back(parsed_double);
            }
            catch (...)
            {
                continue;
            }

            last_delim_pos = l;
        }
    }
}

// проверка на наличие списка значений в строке до конца секции
inline bool is_can_parse_values(const std::string& actual_substr)
{
    if (actual_substr.size() == 0)
        return false;

    const size_t sequence_delim_pos = actual_substr.find(',');
    const size_t sequence_start_section_pos = actual_substr.find('[');
    const size_t sequence_end_section_pos = actual_substr.find(']');

    const bool is_first_literal_delim = actual_substr[0] == COMMA_DELIM || actual_substr[0] == START_SECTION_DELIM || actual_substr[0] == END_SECTION_DELIM || actual_substr[0] == DOUBLE_QUOTE_DELIM;

    const bool is_first_literal_number_or_sign = actual_substr[0] == '0' ||
        actual_substr[0] == '1' ||
        actual_substr[0] == '2' ||
        actual_substr[0] == '3' ||
        actual_substr[0] == '4' ||
        actual_substr[0] == '5' ||
        actual_substr[0] == '6' ||
        actual_substr[0] == '7' ||
        actual_substr[0] == '8' ||
        actual_substr[0] == '9' || actual_substr[0] == '-';

    std::string test_values_str_repr = actual_substr;
    std::replace(test_values_str_repr.begin(), test_values_str_repr.end(), '[', COMMA_DELIM);
    std::replace(test_values_str_repr.begin(), test_values_str_repr.end(), ']', COMMA_DELIM);

    std::string section_values = test_values_str_repr;

    std::vector<double> values;
    parse_values(section_values, values);

    return values.size() > 0 && !is_first_literal_delim && is_first_literal_number_or_sign;
}

// проверка на наличие в строке не символьных литералов / разделителей
bool is_string_without_delimeters(const std::string& actual_substr)
{
    if (actual_substr.size() == 0)
        return true;

    bool is_success = false;

    for (size_t k = 0; k < actual_substr.size(); k++)
    {
        if (actual_substr[k] == COMMA_DELIM 
            || actual_substr[k] == START_SECTION_DELIM 
            || actual_substr[k] == END_SECTION_DELIM 
            || actual_substr[k] == DOUBLE_QUOTE_DELIM
            || actual_substr[k] == DOT_DELIM)
            return is_success;
    }

    is_success = true;
    return is_success;
}

// считываем список значений для секции
void try_parse_values_from_section(const std::string& actual_substr, std::pair<std::string, sectionValues>& data)
{
    if (actual_substr.size() == 0)
        return;

    std::string test_values_str_repr = actual_substr;
    std::replace(test_values_str_repr.begin(), test_values_str_repr.end(), '[', COMMA_DELIM);
    std::replace(test_values_str_repr.begin(), test_values_str_repr.end(), ']', COMMA_DELIM);

    std::string section_values = test_values_str_repr;
    std::vector<double> values;
    parse_values(section_values, values);

    if (values.size() != 0 && data.second.section_values.size() == 0)
            data.second.section_values = values;
}

// считываем значение строки после начала секции для трех случаев
// когда секция простая, вида: PARAMETER[\"NO_PROJECTION\"]
// когда секция сложная (со значениями) или сложная и вложенная: PRIMEM[\"Greenwich\",0.0], DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.000000,298.257224]]
void try_parse_section_value(const std::string& actual_substr, std::pair<std::string, sectionValues>& data)
{
    if (actual_substr.size() == 0)
        return;

    const size_t sequence_delim_pos = actual_substr.find(',');

    const std::string value_complex_or_complex_nested = actual_substr.substr(1, sequence_delim_pos - 2);
    const std::string value_simple = actual_substr.substr(1, sequence_delim_pos - 3);

    if (value_complex_or_complex_nested.size() == 0 && value_simple.size())
        return;

    const bool is_value_complex_or_complex_nested = is_string_without_delimeters(value_complex_or_complex_nested);
    const bool is_value_simple = is_string_without_delimeters(value_simple);

    if (is_value_complex_or_complex_nested && is_value_simple)
    {
        if (data.second.section_value.size() == 0)
            data.second.section_value = value_complex_or_complex_nested.size() > value_simple.size() ? value_complex_or_complex_nested : value_simple;
    }
    else if (is_value_complex_or_complex_nested)
    {
        if(data.second.section_value.size() == 0)
            data.second.section_value = value_complex_or_complex_nested;
    }
    else if (!is_value_complex_or_complex_nested && is_value_simple)
    {
        if (data.second.section_value.size() == 0)
            data.second.section_value = value_simple;
    }
}

// обрезаем кусок строки конфига до начала следующей секции
bool trim_shp_config_before_section(std::string& actual_substr)
{
    if (actual_substr.size() == 0)
        return false;

    bool is_success = false;
    const size_t sequence_delim_pos = actual_substr.find(',');

    if (sequence_delim_pos != std::string::npos)
    {
        is_success = true;
        actual_substr.erase(0, sequence_delim_pos + 1);
    }
    else
    {
        is_success = false;
    }

    return is_success;
}

// считываем конец строки конфига шейпа
void try_parse_tail(std::string& actual_substr, std::vector<std::pair<std::string, sectionValues>>& sections_data)
{
    if (actual_substr.size() == 0 || sections_data.size() == 0)
        return;

    std::string tail_substring = actual_substr;
    size_t pos = 0;

    //"Lambert_Conformal_Conic"]] -> нет запятых -> простая секция
    //"Greenwich",0.0]] -> есть одна запятая, сложная секция
    //"WGS_1984",6378137.000000,298.257224]]] -> больше одной запятых -> сложная секция со значениями

    long delimeters_count = 0;
    for (size_t t = 0; t < tail_substring.size(); t++)
    {
        if (tail_substring[t] == ',')
            delimeters_count += 1;
    }

    if (delimeters_count == 0)
    {
        pos = tail_substring.find(']');
        if (pos != std::string::npos)
            sections_data.back().second.section_value = tail_substring.substr(1, pos - 2);
    }
    else if (delimeters_count >= 1)
    {
        try_parse_section_value(tail_substring, sections_data.back());

        pos = tail_substring.find(',');
        tail_substring = tail_substring.substr(pos);
        try_parse_values_from_section(tail_substring, sections_data.back());
    }
}

//задаем тип секции по наличию у нее значений
void set_sections_type(std::vector<std::pair<std::string, sectionValues>>& sections_data)
{
    if (sections_data.size() == 0)
        return;

    for (auto& section_packed_data : sections_data)
    {
        if (section_packed_data.second.section_values.size() == 0)
            section_packed_data.second.is_simple_or_nested = true;
        else
            section_packed_data.second.is_simple_or_nested = false;
    }

    // первая секция уникальная - корневая, но несмотря на ее вложенность, напрямую списка значений она не содержит
}

void try_parse_shp_prj(CString _inputStrConfig, SHPConfigPrj& data)
{
    // преобразование из TCHAR в LPCSTR
    CT2CA pszConvertedAnsiString(_inputStrConfig);
    // используем конструктов строки для типа LPCSTR
    std::string inputStrConfig(pszConvertedAnsiString);

    size_t pos = 0;

    std::string actual_substr;
    std::vector<std::pair<std::string, sectionValues>> sections_data;

    // инициализация значения initial_config исходным конфигом шейпа
    data.initial_config = inputStrConfig;

    std::pair<std::string, sectionValues> section_data;
    while ((pos = inputStrConfig.find('[')) != std::string::npos)
    {
        actual_substr = inputStrConfig.substr(0, pos);
        bool is_need_parse_section_value = is_starting_section_value(inputStrConfig);
        bool is_need_parse_values = is_can_parse_values(actual_substr);

        if (is_string_without_delimeters(actual_substr))
        {
            section_data.first = actual_substr;
            section_data.second.section_name = actual_substr;

            sections_data.push_back(section_data);
        }

        if (is_need_parse_values)
        {
            if (sections_data.size() != 0)
                try_parse_values_from_section(inputStrConfig, sections_data.back());
        }

        if (is_need_parse_section_value)
        {
            if (sections_data.size() != 0)
                try_parse_section_value(inputStrConfig, sections_data.back());

            if (trim_shp_config_before_section(inputStrConfig))
                continue;
        }

        inputStrConfig.erase(0, pos + 1);
    }

    try_parse_tail(inputStrConfig, sections_data);

    set_sections_type(sections_data);

    data.data = sections_data;
}

int main()
{
    SHPConfigPrj parsed_data;
	
    //типы данных arcgis и точность
    //https://pro.arcgis.com/ru/pro-app/latest/help/data/geodatabases/overview/arcgis-field-data-types.htm

    //- если секция имеет вид UNIT3[\"Kilometer\",], то секции будет задан флаг простой
    //- все численные представления парсятся как double
    //- строка конфига должна быть без пробелов, иначе какие-то значения могут считаться неверно

    //CString testStr = "GEOGCS[\"GCS_WGS_1984\",PRIMEM[\"Greenwich\",0.0]]";
    //CString testStr = "GEOGCS[\"GCS_WGS_1984\",UNIT[\"T\",10.0],PARAMETER[\"False_Easting\",500000.0]]";
    //CString testStr = "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.000000,298.257224]],PRIMEM[\"Greenwich\",0.0],UNIT[\"Kilometer\",1000.0]]";
    CString testStr = "GEOGCS[\"GCS_Pulkovo_1942\",DATUM[\"D_Pulkovo_1942\",SPHEROID[\"Krasovsky_1940\",6378245.0,298.3]],PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433,,666.0010098,1.0]]";
    //CString testStr = "GEOGCS[\"GCS_WGS_1984\",UNIT1[\"Kilometer\",UNIT2[\"R\",UNIT3[\"Kilometer\",10.0,,-23.0,45,-90,,,90]]]]";
    //CString testStr = "GEOGCS[\"GCS_WGS_1984\",UNIT1[\"Kilometer\",UNIT2[\"Rr\",UNIT3[\"Kilometer\",10.0,,-23.0,45,-90]]],DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.000000,298.257224]],UNIT[\"Kilometer\",1000.0],PARAMETER2[\"Central_Meridian_Test\",-124.5],PROJECTION[\"Lambert_Conformal_Conic\"],PRIMEM[\"Greenwich\",0.0],PROJECTION2[\"Lambert_Conformal_Conic_Test\"],PROJECTION[\"Lambert_Conformal_Conic\"]]";
    //CString testStr = "GEOGCS[\"GCS_WGS_1984\",PROJECTION[\"Lambert_Conformal_Conic\"],PRIMEM[\"Greenwich2\",0.0]]";

    try_parse_shp_prj(testStr, parsed_data);

    parsed_data.update_section_value_by_section_name("SPHEROID", "D_ITRF_2008");
    std::vector<double> values_for_test;

    //values_for_test.push_back(-1.00);
    //values_for_test.push_back(21.00);
    //values_for_test.push_back(1.23458891456789565456654543);
    //values_for_test.push_back(-1.0010592323288999643499953);
    //values_for_test.push_back(-100.0010592323288999643499953);

    //values_for_test.push_back(12);
    //values_for_test.push_back(-0.298);
    //parsed_data.update_values_by_section_name("SPHEROID", values_for_test);

    //values_for_test.clear();
    //values_for_test.push_back(6378245.0);
    //values_for_test.push_back(298.3);
    //parsed_data.update_values_by_section_name("SPHEROID", values_for_test);

    //values_for_test.clear();
    //values_for_test.push_back(178.32);
    //parsed_data.update_values_by_section_name("PRIMEM", values_for_test);

    parsed_data.update_section_value_by_index("SPHEROID", -0.1234222, 1);

    return 0;
}