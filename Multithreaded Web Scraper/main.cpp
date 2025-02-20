#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <curl/curl.h>
#include <gumbo.h>

std::mutex coutMutex;

class Guard {
private:
    std::thread* thrd;
    bool detachOnDestroy = false;
public:
    explicit Guard(std::thread& thread, bool detach = false) : thrd(&thread), detachOnDestroy(detach) {}

    Guard(Guard&& other) noexcept : thrd(other.thrd), detachOnDestroy(other.detachOnDestroy) {
        other.thrd = nullptr;
    }

    ~Guard() {
        if (thrd && thrd->joinable()) {
            try {
                if (!detachOnDestroy) {
                    thrd->join();
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Thread Joined\n";
                }
                else {
                    thrd->detach();
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Thread Detached\n";
                }
            }
            catch (const std::system_error& e) {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cerr << "Thread operation failed: " << e.what() << '\n';
            }
        }
    }

    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
    Guard& operator=(Guard&&) = delete;
};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    size_t totalSize = size * nmemb;
    data->append((char*)contents, totalSize);
    return totalSize;
}

std::string fetchPage(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string html;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cerr << "curl_easy_perform() failed for " << url << ": "
                << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    return html;
}

void extractLinks(GumboNode* node, std::vector<std::string>& links) {
    if (node->type != GUMBO_NODE_ELEMENT) return;

    if (node->v.element.tag == GUMBO_TAG_A) {
        GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
        if (href) {
            links.push_back(href->value);
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        extractLinks(static_cast<GumboNode*>(children->data[i]), links);
    }
}

std::string getDomainFromUrl(const std::string& url) {
    std::string domain;
    size_t start = url.find("://") + 3;
    size_t end = url.find('/', start);
    if (end == std::string::npos) {
        end = url.length();
    }
    domain = url.substr(start, end - start);
    for (char& c : domain) {
        if (c == '.' || c == ':') c = '_';
    }
    return domain;
}

void saveHtmlToFile(const std::string& html, const std::string& filename) {
    std::ofstream outFile(filename + ".html");
    if (outFile.is_open()) {
        outFile << html;
        outFile.close();
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Saved HTML to " << filename << ".html" << std::endl;
    }
    else {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cerr << "Failed to save HTML to " << filename << ".html" << std::endl;
    }
}

void processLink(const std::string& link) {
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Fetching link: " << link << std::endl;
    }
    std::string linkHtml = fetchPage(link);
    if (!linkHtml.empty()) {
        std::string domain = getDomainFromUrl(link);
        saveHtmlToFile(linkHtml, domain);
    }
    else {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Skipping " << link << " due to fetch failure" << std::endl;
    }
}

int main() {
	std::string url = "https://example.com"; std::cin >> url;
    std::string html = fetchPage(url);

    if (html.empty()) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Failed to fetch page!" << std::endl;
        return 1;
    }

    GumboOutput* output = gumbo_parse(html.c_str());
    std::vector<std::string> links;

    extractLinks(output->root, links);

    std::vector<std::thread> threads;
    std::vector<Guard> guards;

    for (const auto& link : links) {
        threads.emplace_back(processLink, link);
        guards.emplace_back(threads.back(), false);
    }

    gumbo_destroy_output(&kGumboDefaultOptions, output);

    return 0;
}