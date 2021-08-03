#pragma once
#include "framework.h";
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <fstream>
#include "Calibrator.h"


template <typename T>
std::vector<T> selectIndices(std::vector<T> vec, std::vector<int> indices)
{
    std::vector<T> result_vec = std::vector<T>(indices.size());
    std::transform(indices.begin(), indices.end(), result_vec.begin(), [vec](size_t pos) {return vec[pos]; });
    return result_vec;
}

template <typename T>
void extend(std::vector<T>& vector1, std::vector<T>& vector2) {
    vector1.reserve(vector1.size() + std::distance(vector2.begin(), vector2.end()));
    vector1.insert(vector1.end(), vector2.begin(), vector2.end());
}

class DelaunayCalibrator : public Calibrator {
private:
    cv::Rect img_rect;
    cv::Subdiv2D actualMesh;
    cv::Subdiv2D predictedMesh;
    bool status;
    float margin = 0.10;
    float ratio;
    int img_width = 600;
    cv::Point2f orig;
    std::vector<cv::Point2f> default_coordinates;

public:
    DelaunayCalibrator(cv::Rect rect) {
        initDelaunaySpace(rect, this->margin);
        this->add(this->default_coordinates, this->default_coordinates, false);
    }
    ~DelaunayCalibrator() {}

    void reset() override final {
        this->actual_coordinates = std::vector<cv::Point2f>();
        this->predicted_coordinates = std::vector<cv::Point2f>();
        this->actualMesh = cv::Subdiv2D(this->rect);
        this->predictedMesh = cv::Subdiv2D(this->rect);
    }

    void add(cv::Point2f actualCoordinate, cv::Point2f predictedCoordinate, bool remap = true) override final {
        // Convert to Delaunay Space
        if (remap) {
            actualCoordinate = InputToDelaunay(actualCoordinate);
            predictedCoordinate = InputToDelaunay(predictedCoordinate);
        }
        actualCoordinate = limitXY(actualCoordinate);
        predictedCoordinate = limitXY(predictedCoordinate);

        this->actual_coordinates.push_back(actualCoordinate);
        this->predicted_coordinates.push_back(predictedCoordinate);
        this->actualMesh.insert(actualCoordinate);
        this->predictedMesh.insert(predictedCoordinate);
    }

    void add(std::vector<cv::Point2f> actualCoordinates, std::vector<cv::Point2f> predictedCoordinates, bool remap = true) override final {
        // Convert to Delaunay Space
        if (remap) {
            actualCoordinates = InputToDelaunay(actualCoordinates);
            predictedCoordinates = InputToDelaunay(predictedCoordinates);
        }
        actualCoordinates = limitXY(actualCoordinates);
        predictedCoordinates = limitXY(predictedCoordinates);

        // Add to the coordinate vector
        extend(this->actual_coordinates, actualCoordinates);
        extend(this->predicted_coordinates, predictedCoordinates);
        // Add to the vector
        this->actualMesh.insert(actualCoordinates);
        this->predictedMesh.insert(predictedCoordinates);
    }

    cv::Point2f evaluate(cv::Point2f searchPoint) override final {
        if (this->actual_coordinates.size() <= 3)
            return searchPoint;

        // Map to Delaunay space
        searchPoint = InputToDelaunay(searchPoint);
        cv::Vec6f t = findTriangle(searchPoint, this->predictedMesh);

        std::vector<cv::Point2f> predicted_vertices = getTriangleVertices(t);
        std::vector<int> predicted_vertices_indices = findIndices(this->predicted_coordinates, predicted_vertices);
        std::vector<cv::Point2f> actual_vertices = selectIndices(this->actual_coordinates, predicted_vertices_indices);
        cv::Mat M = cv::getAffineTransform(predicted_vertices, actual_vertices);

        std::vector<cv::Point2f> calibrated_vector;
        cv::transform(std::vector<cv::Point2f>{ searchPoint }, calibrated_vector, M);
        //LOG_DEBUG("########## %d ###########", this->actual_coordinates.size());
        return DelaunayToInput(calibrated_vector[0]);
    }

    // Fix problem with img creation
    void drawDistortionMap() override final {
        // Keep the image size small
        cv::Mat img(img_rect.size(), CV_8UC3);
        img = cv::Scalar::all(0);
        //cv::Mat img = cv::Mat(cv::Size(this->rect.width, this->rect.height), CV_32FC3);
        drawDelaunay(img, this->actualMesh, cv::Scalar(0, 255, 0) /*Green*/);
        drawDelaunay(img, this->predictedMesh, cv::Scalar(255, 0, 0)/*Blue*/);
        cv::imshow("DelaunayDistortionMap", img);
        cv::waitKey(1);
    }

    void initDelaunaySpace(cv::Rect rect, float margin=0.10) {
        this->margin = margin;
        float stretch = 1.0 + (2 * margin);
        this->orig = cv::Point2f(rect.width, rect.height);
        this->rect = cv::Rect(0, 0, stretch * this->orig.x, stretch * this->orig.y);
        this->actualMesh = cv::Subdiv2D(this->rect);
        this->predictedMesh = cv::Subdiv2D(this->rect);

        this->default_coordinates = {
                                        cv::Point2f(0, 0),
                                        cv::Point2f(this->rect.width, 0),
                                        cv::Point2f(this->rect.width, this->rect.height),
                                        cv::Point2f(0, this->rect.height)

        };
        this->actual_coordinates = std::vector<cv::Point2f>();
        this->predicted_coordinates = std::vector<cv::Point2f>();
        this->ratio = (float)this->img_width / this->rect.width;
        this->img_rect = cv::Rect(0, 0, this->rect.width * ratio, this->rect.height * ratio);
    }

    std::vector<cv::Point2f> InputToDelaunay(std::vector<cv::Point2f> in_vector) {
        // Map to Delaunay space
        std::vector<cv::Point2f> out_vector;
        for (auto& point : in_vector) {
            out_vector.push_back(InputToDelaunay(point));
        }
        return out_vector;
    }

    cv::Point2f InputToDelaunay(cv::Point2f point) {
        // Map to Delaunay space
        return cv::Point2f(point.x + this->orig.x * this->margin, point.y + this->orig.y * this->margin);
    }

    cv::Point2f DelaunayToInput(cv::Point2f point) {
        // Map to Input space
        return cv::Point2f(point.x - this->orig.x * this->margin, point.y - this->orig.y * this->margin);
    }

    std::vector<cv::Point2d> convertToInt(std::vector<cv::Point2f> data) {
        std::vector<cv::Point2d> data_int;
        for (auto& item : data) {
            data_int.push_back(cv::Point2d(int(item.x), int(item.y)));
        }
        return data_int;
    }

    std::vector<cv::Point2f> limitXY(std::vector<cv::Point2f> coordinates) {
        std::vector<cv::Point2f> corrected_coordinates = std::vector<cv::Point2f>();
        for (auto& point : coordinates) {
            corrected_coordinates.push_back(limitXY(point));
        }
        return corrected_coordinates;
    }

    cv::Point2f limitXY(cv::Point2f point) {
            float W = this->rect.width;
            float H = this->rect.height;
            float origX = this->rect.x;
            float origY = this->rect.y;
            float x = std::max(std::min(W - 1, point.x), origX);
            float y = std::max(std::min(H - 1, point.y), origY);
            return cv::Point2f(x, y);
    }

    cv::Subdiv2D createDelaunayMesh(std::vector<cv::Point2f> coordinates) {
        // Initialize Subdivision
        cv::Subdiv2D mesh = cv::Subdiv2D(this->rect);
        mesh.insert(coordinates);
        return mesh;
    }

    std::vector<cv::Point2f> getTriangleVertices(cv::Vec6f &triangle, float ratio=1.0f) {
        std::vector<cv::Point2f> vertices(3);
        // Create vector of vertices
        for (int i = 0; i < vertices.size(); i++) {
            vertices[i] = cv::Point2f(triangle[2 * i] * ratio, triangle[2 * i + 1] * ratio);
        }
        return vertices;
    }

    bool contains(std::vector<cv::Point2f> vertices, cv::Point2f vertex) {
        if (std::find(vertices.begin(), vertices.end(), vertex) != vertices.end())
            return true;
        else
            return false;
    }

    std::vector<int> findIndices(std::vector<cv::Point2f> all_vertices, std::vector<cv::Point2f> search_vertices) {
        std::vector<int> indices;
        for (auto& vertex : search_vertices) {
            auto iter = std::find(all_vertices.begin(), all_vertices.end(), vertex);
            indices.push_back(std::distance(all_vertices.begin(), iter));
        }
        return indices;
    }

    cv::Vec6f findTriangle(cv::Point2f p, cv::Subdiv2D mesh) {
        // get the full triangle list
        std::vector<cv::Vec6f> triangleList;
        mesh.getTriangleList(triangleList);

        cv::Vec6f output;

        // Find an edge near the searchPoint
        int edgeId;
        int vertexId;
        int retval;

        try {
            // handle if p is outside the rect/mesh
            retval = mesh.locate(p, edgeId, vertexId);
            if (retval == cv::Subdiv2D::PTLOC_INSIDE || retval == cv::Subdiv2D::PTLOC_ON_EDGE) {
                cv::Point2f v1;
                cv::Point2f v2;
                mesh.edgeOrg(edgeId, &v1);
                mesh.edgeDst(edgeId, &v2);

                // filter the two triangles that share the above edge
                // and see if the point is inside the triangle
                for (auto& t : triangleList) {
                    // Create vector of vertices
                    std::vector<cv::Point2f> vertices = getTriangleVertices(t);
                    if (contains(vertices, v1) && contains(vertices, v2)) {

                        if (retval == cv::Subdiv2D::PTLOC_ON_EDGE) {
                            return t;
                        }
                        else {
                            bool b1 = (sign(p, vertices[0], vertices[1]) < 0.0);
                            bool b2 = (sign(p, vertices[1], vertices[2]) < 0.0);
                            bool b3 = (sign(p, vertices[2], vertices[0]) < 0.0);
                            if (b1 == b2 && b2 == b3) {
                                return t;
                            }
                        }
                    }
                }
            }
            else if (retval == cv::Subdiv2D::PTLOC_VERTEX) {
                cv::Point2f vertex = mesh.getVertex(vertexId);
                for (cv::Vec6f t : triangleList) {
                    std::vector<cv::Point2f> vertices = getTriangleVertices(t);
                    // (vertex in vertices)
                    if (contains(vertices, vertex)) {
                        return t;
                    }
                }
            }
            else if (retval == cv::Subdiv2D::PTLOC_OUTSIDE_RECT) {
                return output;
            }
        }
        catch (std::exception& e) {
            LOG_ERROR("%s", e.what());
        }
        return output;
    }

    float sign(cv::Point2f a, cv::Point2f b, cv::Point2f c) {
        return (a.x - c.x) * (b.y - c.y) - (b.x - c.x) * (a.y - c.y);
    }



    // Draw delaunay triangles
    void drawDelaunay(cv::Mat img, cv::Subdiv2D mesh, cv::Scalar color){
        std::vector<cv::Vec6f> triangleList;
        mesh.getTriangleList(triangleList);
        for (auto& t : triangleList) {
            drawTriangle(img, t, color);
        }
    }

    // Draw a triangle
    void drawTriangle(cv::Mat img, cv::Vec6f t, cv::Scalar color) {
        std::vector<cv::Point2f> vertices = getTriangleVertices(t, this->ratio);
        if (rect_contains(vertices[0]) && rect_contains(vertices[0]) && rect_contains(vertices[2])) {
            cv::circle(img, vertices[0], 2, color, -1);
            cv::circle(img, vertices[1], 2, color, -1);
            cv::circle(img, vertices[2], 2, color, -1);
            cv::line(img, vertices[0], vertices[1], color, 1, cv::LINE_AA, 0);
            cv::line(img, vertices[1], vertices[2], color, 1, cv::LINE_AA, 0);
            cv::line(img, vertices[2], vertices[0], color, 1, cv::LINE_AA, 0);
        }
    }

    // Check if a point is inside the base rectangle
    bool rect_contains(cv::Point2f point) {
        if (this->img_rect.x <= point.x && point.x <= this->img_rect.width &&
            this->img_rect.y <= point.y && point.y <= this->img_rect.height) {
            return true;
        }
        else {
            return false;
        }
    }

    bool serialize(const wchar_t* path) override final {

        std::ofstream out(path);

        //std::ofstream out;
        //out.open(path, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!out.is_open())
            return false;

        out << this->actual_coordinates.size(); //serialize size of string
        
        out << ';'; //seperator
        for (int i = 0; i < this->actual_coordinates.size(); i++) {
            out << '['; 
            out << this->actual_coordinates[i].x; 
            out << ','; //number seperator
            out << this->actual_coordinates[i].y;
            out << ']'; 
        }

        out << ';'; //seperator
        for (int i = 0; i < this->predicted_coordinates.size(); i++) {
            out << '['; 
            out << this->predicted_coordinates[i].x;
            out << ','; //number seperator
            out << this->predicted_coordinates[i].y;
            out << ']';
        }
        
        return true;
    }

    bool deserialize(const wchar_t* path) override final {

        std::ifstream in(path);

        //std::ifstream in;
        //in.open(path, std::ios::in | std::ios::binary | std::ios::ate);

        if (!in.is_open())
            return false;

        int len = 0;
        char sep;
        float x, y;
        in >> len;  //deserialize size of vector
        std::vector<cv::Point2f> actual(len), predicted(len);
        in >> sep; //read in the ;
        for (int i = 0; i < len; i++) {
            in >> sep; //read in the [
            in >> x;
            in >> sep; //read in the ,
            in >> y;
            in >> sep; //read in the ]
            actual[i] = cv::Point2f(x, y);
        }

        in >> sep; //read in the ;
        for (int i = 0; i < len; i++) {
            in >> sep; //read in the [
            in >> x;
            in >> sep; //read in the ,
            in >> y;
            in >> sep; //read in the ]
            predicted[i] = cv::Point2f(x, y);
        }
        // No remapping required as the coordinates are already mapped to DelaunaySpace
        this->reset();
        this->add(actual, predicted, false);
        return true;
    }

};
