#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> // for std::exit
#include <algorithm> 

#include <openssl/ssl.h>
#include <httplib.h>
#include <nlohmann\json.hpp>


void saveUsername(const std::string& username) {
    std::ofstream out("config.config");
    if (out.is_open()) {
        out << "username=" << username;
        out.close();
    }
    else {
        std::cerr << "Unable to open config file for writing!" << std::endl;
    }
}

std::string loadUsername() {
    std::ifstream in("config.config");
    std::string username;
    if (in.is_open()) {
        std::string line;
        while (std::getline(in, line)) {
            username = line.substr(9); // Get the username value from the line read from the file
        }
        in.close();
        return username;
    }
    else {
        std::cerr << "Unable to open config file for reading!" << std::endl;
    }
    return username; // Returns an empty string if the username is not found in the config file
}


std::string fetchToken(std::string username_, std::string password_) {

    httplib::SSLClient client("www.capitalsheets.com");

    httplib::Params params;
    params.emplace("username", username_);
    params.emplace("password", password_);

    auto res = client.Post("/api_login/", params);
    std::cout << res << std::endl;
    if (res && res->status == 200) {
        try {
            auto json_response = nlohmann::json::parse(res->body);
            std::string token = json_response.at("token").get<std::string>();
            std::cout << "token received: " << token << std::endl;
            return token;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to parse! :(" << std::endl;
            return {};
        }
    }
    else {
        std::cerr << "Failed to login!" << std::endl;
        return{};
    }
};

std::pair <bool, std::string> ShowLoginWindow(bool* p_open){

    static bool isAuthenticated = false;
    static char buf0[20] = "";
    static char buf1[20] = "";
    static std::string token;

    // This will load the username only the first time this function runs
    static bool isUsernameLoaded = false;
    if (!isUsernameLoaded) {
        std::string savedUsername = loadUsername();
        strncpy_s(buf0, savedUsername.c_str(), sizeof(buf0) - 1);
        isUsernameLoaded = true; // To make sure we don't load it again
    }



    if (!ImGui::Begin("Login", p_open, isAuthenticated ? ImGuiWindowFlags_NoInputs : 0)) // Block inputs to other windows when not authenticated
    {
        ImGui::End();
        return { isAuthenticated , token};
    }

    if (!isAuthenticated) // Show login only if not authenticated
    {
        ImGui::InputText("username", buf0, sizeof(buf0));
        ImGui::InputText("password", buf1, sizeof(buf1), ImGuiInputTextFlags_Password);

        static bool rememberMe = true;
        ImGui::Checkbox("Remember Me", &rememberMe); // This will create a checkbox in ImGui window

        if (ImGui::Button("Login"))
        {
            std::string usernameStr(buf0);
            std::string passwordStr(buf1);
            token = fetchToken(usernameStr, passwordStr);
            if (!token.empty())
            {
                isAuthenticated = true; // Set to true on successful login

                if (rememberMe) {
                    saveUsername(usernameStr); // Save username when "Remember Me" is checked
                }
                else {
                    saveUsername(""); // Clear saved username when "Remember Me" is unchecked
                }
            }
        }
        
    }
    ImGui::End();
    return { isAuthenticated, token };
}

void dowloadAllBrowser(const std::string& ticker) {
    std::string CapitalAllTicker = ticker;

    std::transform(CapitalAllTicker.begin(), CapitalAllTicker.end(), CapitalAllTicker.begin(), ::toupper);

    std::string url = "https://www.capitalsheets.com/static/downloads/" + CapitalAllTicker + "%20Financial%20Statements.xlsx";


    #ifdef _WIN32
        std::string command = "start " + url;

    #elif defined(__APPLE__)
        std::string command = "open " + url;
    #else
        std::string command = "xdg-open " + url;
    #endif

        std::system(command.c_str());
    }

void downloadViaBrowser(const std::string& ticker, const std::string& dataType) {

    std::string CapitalTicker = ticker;

    std::transform(CapitalTicker.begin(), CapitalTicker.end(), CapitalTicker.begin(), ::toupper);

    std::string url = "https://www.capitalsheets.com/static/downloads/" + CapitalTicker + "%20" + dataType + "%20Sheets" + ".csv";

    #ifdef _WIN32
        std::string command = "start " + url;

    #elif defined(__APPLE__)
        std::string command = "open " + url;
    #else
        std::string command = "xdg-open " + url;
    #endif

        std::system(command.c_str());
    }



void processApiData(const nlohmann::json& apiData, std::string& firstValue, std::vector<double>& columnYears, std::vector<double>& rowData) {

    rowData.clear();
    columnYears.clear();
    firstValue.clear();

    // Ensure the input JSON is not empty
    if (apiData.empty()) {
        return;
    }

    auto it = apiData.begin();

    // Capture the first value if it's a string
    if (it.value().is_string()) {
        firstValue = it.value().get<std::string>();
    }

    ++it;  // Skip the first key

    // Iterate over the keys (columns) of the JSON object
    for (; it != apiData.end(); ++it) {

        std::string yearStr = it.key().substr(5);
        double real_year = std::stod(yearStr);
        columnYears.push_back(real_year);

        if (it.value() == "-") {
            rowData.push_back(0.0);

        }
        else {
            std::string dataStr = it.value();
            double real_data = std::stod(dataStr);
            rowData.push_back(real_data);
        }
    }
}

std::string Capitalize(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Display the JSON data as an ImGui table
void displayDataAsTable(const nlohmann::json& data, std::vector<double>& columnYears, std::vector<double>& rowData, std::string& firstValue) {
    if (data.empty()) return;

    static int selected_row = -1;


    const auto& firstRow = data[0];
    int numColumns = firstRow.size();

    if (ImGui::BeginTable("data_table", numColumns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX)) {

        // Setup all columns with a default width
        for (const auto& [key, value] : firstRow.items()) {
            ImGui::TableSetupColumn(key.c_str(), ImGuiTableColumnFlags_WidthFixed, 150.0f);
        }

        ImGui::TableHeadersRow();  // This will create the header row using the names from TableSetupColumn


        // Rows
        for (int rowIndex = 0; rowIndex < data.size(); rowIndex++) {
            const auto& row = data[rowIndex];

            ImGui::TableNextRow();

            int columnIndex = 0;
            for (const auto& [key, value] : row.items()) {
                ImGui::TableSetColumnIndex(columnIndex);

                std::string label = value.is_string() ? value.get<std::string>() : std::to_string(value.get<double>());

                if (ImGui::Selectable((label + "##" + std::to_string(rowIndex)).c_str(), selected_row == rowIndex, ImGuiSelectableFlags_SpanAllColumns)) {
                    selected_row = rowIndex;
                    // Process the selected row data
                    processApiData(data[selected_row], firstValue, columnYears, rowData);
                }
                columnIndex++;
            }
        }
        ImGui::EndTable();
    }
}


nlohmann::json fetchDataFromEndpoint(std::string endpoint, std::string ticker, std::string token) {

    // Convert the ticker to lowercase
    std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);

    httplib::SSLClient client("www.capitalsheets.com");
    httplib::Headers headers = { {"Authorization", "Token " + token} };

    std::string path = "/api/" + std::string(ticker) + endpoint + "/?format=json";

    auto response = client.Get(path.c_str(), headers);

    if (response && response->status == 200) {

        try {
            return nlohmann::json::parse(response->body);
        }
        catch (const std::exception& e) {
            //std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
            //std::cerr << "Received JSON body: " << response->body << std::endl;
            return {};  // Return an empty JSON object
        }
    }
    else {

        //std::cerr << "Failed to fetch data. HTTP Status Code: " << (response ? std::to_string(response->status) : "unknown") << std::endl;
        return {};  // Return an empty JSON object
    }
}

nlohmann::json fetchDataForBalanceSheets(const char* ticker, std::string token) {
    return fetchDataFromEndpoint("bs", ticker, token);
}

nlohmann::json fetchDataForIncomeSheets(const char* ticker, std::string token) {
    return fetchDataFromEndpoint("is", ticker, token);
}

nlohmann::json fetchDataForCashflowSheets(const char* ticker, std::string token) {
    return fetchDataFromEndpoint("cfs", ticker, token);
}

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << std::endl;
    std::exit(EXIT_FAILURE);
}