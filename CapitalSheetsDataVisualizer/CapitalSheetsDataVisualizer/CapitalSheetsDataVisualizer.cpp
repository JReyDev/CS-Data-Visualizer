#include <iostream>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <openssl/ssl.h>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <nlohmann\json.hpp>

#include <csutilities.h>




int main() {

    glfwSetErrorCallback(glfw_error_callback);

    char ticker[7] = "aapl";  //buffer overflow?


    std::vector <double> rowData;
    std::vector <double> columnYears;
    std::string firstValue; 

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 1000, "Capital Sheets Visual", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext(); 

    try {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("lato.ttf", 15);
    }
    catch (const std::exception& e) {
        std::cerr << "Font Error! " << e.what() << std::endl;
    }


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");


    ImGui::StyleColorsDark();


    nlohmann::json balanceSheetData;
    nlohmann::json incomeSheetData;
    nlohmann::json cashflowSheetData;


    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();


        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Tab] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);           
        colors[ImGuiCol_TabActive] = ImVec4(0.53f, 0.79f, 0.98f, 1.0f);    

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        ImVec2 companyDataSize(windowWidth, windowHeight * 0.5f);
        ImVec2 companyDataPos(0, windowHeight * 0.5f);


        ImVec2 plotWindowSize(windowWidth, windowHeight * 0.5f);
        ImVec2 plotWindowPos(0, 0);


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool open = false;

        auto [isAuthenticated, token] = ShowLoginWindow(&open);

        if (isAuthenticated) {

            ImGui::SetNextWindowSize(companyDataSize);
            ImGui::SetNextWindowPos(companyDataPos);

            ImGui::Begin("Enter Ticker Symbol");

            if (ImGui::InputText("Ticker", ticker, IM_ARRAYSIZE(ticker), ImGuiInputTextFlags_EnterReturnsTrue)) {

                std::cout << "token passed: " << token << std::endl;
                balanceSheetData = fetchDataForBalanceSheets(ticker, token);
                incomeSheetData = fetchDataForIncomeSheets(ticker, token);
                cashflowSheetData = fetchDataForCashflowSheets(ticker, token);
            }
            ImGui::SameLine();

            if (ImGui::Button("Download Balance Sheet")) {
                downloadViaBrowser(ticker, "Balance");
            }

            ImGui::SameLine();

            if (ImGui::Button("Download Income Sheet")) {
                downloadViaBrowser(ticker, "Income");
            }

            ImGui::SameLine();

            if (ImGui::Button("Download Cash Flow Sheet")) {
                downloadViaBrowser(ticker, "Cashflow");
            }

            ImGui::SameLine();

            if (ImGui::Button("Download All Sheets")) {
                dowloadAllBrowser(ticker);
            }


            if (ImGui::BeginTabBar("Sheets")) {
                if (ImGui::BeginTabItem("Balance Sheets")) {
                    if (!balanceSheetData.empty()) {
                        displayDataAsTable(balanceSheetData, columnYears, rowData, firstValue);
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Income Sheets")) {
                    if (!incomeSheetData.empty()) {
                        displayDataAsTable(incomeSheetData, columnYears, rowData, firstValue);
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Cashflow Sheets")) {
                    if (!cashflowSheetData.empty()) {
                        displayDataAsTable(cashflowSheetData, columnYears, rowData, firstValue);
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
            ImGui::End();



            std::vector<double>columnYearsDoubles(columnYears.begin(), columnYears.end());

            ImGui::SetNextWindowSize(plotWindowSize);
            ImGui::SetNextWindowPos(plotWindowPos);

            if (ImGui::Begin("Data Visualization", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) { // Create a new window named "Data Visualization"

                ImVec2 availableSpace = ImGui::GetContentRegionAvail();

                ImPlot::SetNextAxesToFit();

                ImPlot::PushStyleVar(ImPlotStyleVar_PlotDefaultSize, ImVec2(30, 600));

                if (ImPlot::BeginPlot("Plot")) {

                    ImPlot::PlotLine(firstValue.c_str(), columnYearsDoubles.data(), rowData.data(), rowData.size());
                    ImPlot::PlotScatter(firstValue.c_str(), columnYearsDoubles.data(), rowData.data(), rowData.size());

                    if (ImPlot::IsPlotHovered()) {

                        ImPlotPoint mousePos = ImPlot::GetPlotMousePos();
                        for (size_t i = 0; i < rowData.size(); ++i) {

                            if (std::abs(mousePos.x - columnYearsDoubles[i]) < 0.5 && std::abs(mousePos.y - rowData[i]) < 3000000) {

                                ImGui::BeginTooltip();
                                ImGui::Text("Year: %d", static_cast<int>(columnYearsDoubles[i]));
                                ImGui::Text("Value: %d", static_cast<int>(rowData[i]));
                                ImGui::EndTooltip();
                            }
                        }
                    }

                    ImPlot::EndPlot();
                }

                ImGui::End();
            }
        }
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImPlot::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

