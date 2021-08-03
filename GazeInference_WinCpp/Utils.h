#include "framework.h"
#include "cv_constants.h"
#include <string>


/* Template methods */
template <typename T>
std::vector<T> slice(std::vector<T> v, std::tuple<int, int> regionBounds)
{
    return std::vector<T>(v.begin() + std::get<0>(regionBounds), v.begin() + std::get<1>(regionBounds) + 1);
}

template <typename T>
std::vector<T> slice(std::vector<T> v, int start, int end)
{
    return std::vector<T>(v.begin() + start, v.begin() + end + 1);
}

template <typename T>
std::vector<T> select(std::vector<T> vec, std::initializer_list<std::size_t> indices)
{
    std::vector<T> result_vec = std::vector<T>(indices.size());
    std::transform(indices.begin(), indices.end(), result_vec.begin(), [vec](size_t pos) {return vec[pos]; });
    return result_vec;
}


template <typename T>
static void softmax(T& input) {
    float rowmax = *std::max_element(input.begin(), input.end());
    std::vector<float> y(input.size());
    float sum = 0.0f;
    for (size_t i = 0; i != input.size(); ++i) {
        sum += y[i] = std::exp(input[i] - rowmax);
    }
    for (size_t i = 0; i != input.size(); ++i) {
        input[i] = y[i] / sum;
    }
}


/* Gets the top K results */
std::vector<std::tuple<std::string, float>> getTopK(std::vector<float> results, std::vector<std::string> labels, int k) {
    // Find the top k probabilities
    std::vector<float> topProbabilities(k);
    std::vector<float> topProbabilityLabelIndexes(k);
    // SqueezeNet returns a list of 1000 options, with probabilities for each, loop through all
    for (uint32_t i = 0; i < results.size(); i++) {
        // is it one of the top k?
        for (int j = 0; j < k; j++) {
            if (results[i] > topProbabilities[j]) {
                topProbabilityLabelIndexes[j] = i;
                topProbabilities[j] = results[i];
                break;
            }
        }
    }

    // Collect the top K the result
    std::vector<std::tuple<std::string, float>> topK(k);
    for (int i = 0; i < k; i++) {
        topK[i] = std::make_tuple(labels[topProbabilityLabelIndexes[i]], topProbabilities[i]);
    }

    return topK;
}

std::vector<std::string> readLabels(std::string& labelFilepath){
    std::vector<std::string> labels;
    std::string line;
    std::ifstream fp(labelFilepath);
    while (std::getline(fp, line))
    {
        labels.push_back(line);
    }
    return labels;
}

