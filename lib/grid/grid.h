#ifndef GRID_H
#define GRID_H

#include "common/bounding_box.h"
#include "common/arrays.h"
#include "dcel/dcel.h"
#include "engine/tricubic.h"

#include "cgal/aabbtree.h"

#define BORDER_PAY 5
#define STD_PAY 0
#define MIN_PAY -10
#define MAX_PAY 500


class Grid : public SerializableObject{
    public:
        Grid();
        Grid(const Eigen::RowVector3i& resolution, const Eigen::MatrixXd& gridCoordinates, const Eigen::VectorXd& signedDistances, const Eigen::RowVector3i& gMin, const Eigen::RowVector3i& gMax);

        unsigned int getResX() const;
        unsigned int getResY() const;
        unsigned int getResZ() const;

        BoundingBox getBoundingBox() const;


        Vec3 getTarget() const;
        void setTarget(const Vec3& value);

        void calculateBorderWeights(const Dcel &d, bool heightfields = false);
        void calculateWeightsAndFreezeKernel(const Dcel& d, double value, bool heightfields = false);
        void calculateFullBoxValues(double (*integralTricubicInterpolation)(const double *&, double, double, double, double, double, double));

        double getValue(const Pointd &p) const;
        double getUnit() const;
        void getMinAndMax(double &min, double &max);

        Pointd getNearestGridPoint(const Pointd& p) const;
        void getCoefficients(const double*& coeffs, const Pointd& p) const;
        double getFullBoxValue(const Pointd&p) const;

        // SerializableObject interface
        void serialize(std::ofstream& binaryFile) const;
        void deserialize(std::ifstream& binaryFile);

    protected:
        Pointd getPoint(unsigned int i, unsigned int j, unsigned int k) const;
        unsigned int getIndex(unsigned int i, unsigned int j, unsigned int k) const;
        double getSignedDistance(unsigned int i, unsigned int j, unsigned int k) const;
        double getWeight(unsigned int i, unsigned int j, unsigned int k) const;
        int getIndexOfCoordinateX(double x) const;
        int getIndexOfCoordinateY(double y) const;
        int getIndexOfCoordinateZ(double z) const;

        void setNeighboroudWeigth(const Pointd&p, double w);
        void getCoefficients(const double*& coeffs, unsigned int i, unsigned int j, unsigned int k) const;

        void setWeightOnCube(unsigned int i, unsigned int j, unsigned int k, double w);

        BoundingBox bb;
        unsigned int resX, resY, resZ;
        Eigen::MatrixXd gridCoordinates;
        Eigen::VectorXd signedDistances;
        Array3D<double> weights;
        Array4D<double> coeffs;
        Array3D<double> fullBoxValues;
        Vec3 target;

};

inline unsigned int Grid::getResZ() const {
    return resZ;
}

inline BoundingBox Grid::getBoundingBox() const {
    return bb;
}

inline unsigned int Grid::getResY() const {
    return resY;
}

inline unsigned int Grid::getResX() const {
    return resX;
}

inline Vec3 Grid::getTarget() const {
    return target;
}

inline void Grid::setTarget(const Vec3& value) {
    target = value;
}

inline double Grid::getUnit() const {
    return gridCoordinates(getIndex(1,0,0), (0)) - gridCoordinates(getIndex(0,0,0), (0));
}

inline Pointd Grid::getNearestGridPoint(const Pointd& p) const{
    unsigned int ind = getIndex(getIndexOfCoordinateX(p.x()), getIndexOfCoordinateY(p.y()), getIndexOfCoordinateZ(p.z()));
    return std::move(Pointd(gridCoordinates(ind,0), gridCoordinates(ind,1), gridCoordinates(ind,2)));
}

inline void Grid::getCoefficients(const double* &coeffs, unsigned int i, unsigned int j, unsigned int k) const {
    coeffs = this->coeffs(i, j, k);
}

inline void Grid::setWeightOnCube(unsigned int i, unsigned int j, unsigned int k, double w) {
    assert(i+1 < resX);
    assert(j+1 < resY);
    assert(k+1 < resZ);
    weights(i  ,j  ,k  ) = w;
    weights(i  ,j  ,k+1) = w;
    weights(i  ,j+1,k  ) = w;
    weights(i  ,j+1,k+1) = w;
    weights(i+1,j  ,k  ) = w;
    weights(i+1,j  ,k+1) = w;
    weights(i+1,j+1,k  ) = w;
    weights(i+1,j+1,k+1) = w;
}

inline void Grid::getCoefficients(const double* &coeffs, const Pointd& p) const {
    if(bb.isStrictlyIntern(p))
        coeffs = this->coeffs(getIndexOfCoordinateX(p.x()), getIndexOfCoordinateY(p.y()), getIndexOfCoordinateZ(p.z()));
    else coeffs = this->coeffs(0, 0, 0);
}

inline double Grid::getFullBoxValue(const Pointd& p) const {
    if(bb.isStrictlyIntern(p))
        return fullBoxValues(getIndexOfCoordinateX(p.x()), getIndexOfCoordinateY(p.y()), getIndexOfCoordinateZ(p.z()));
    else return fullBoxValues(0,0,0);
}

inline Pointd Grid::getPoint(unsigned int i, unsigned int j, unsigned int k) const {
    unsigned int ind = getIndex(i,j,k);
    return std::move(Pointd(gridCoordinates(ind,0), gridCoordinates(ind,1), gridCoordinates(ind,2)));
}

inline unsigned int Grid::getIndex(unsigned int i, unsigned int j, unsigned int k) const {
    assert (i < resX);
    assert (j < resY);
    assert (k < resZ);
    //return i+resX*(j + resY*k);
    return k+resZ*(j + resY*i);
}

inline double Grid::getSignedDistance(unsigned int i, unsigned int j, unsigned int k) const {
    return signedDistances(getIndex(i,j,k));
}

inline double Grid::getWeight(unsigned int i, unsigned int j, unsigned int k) const {
    return weights(i,j,k);
}

inline int Grid::getIndexOfCoordinateX(double x) const {
    double deltabb = bb.getMaxX() - bb.getMinX();
    double deltax = x - bb.getMinX();
    return (deltax * (resX-1)) / deltabb;
}

inline int Grid::getIndexOfCoordinateY(double y) const {
    double deltabb = bb.getMaxY() - bb.getMinY();
    double deltay = y - bb.getMinY();
    return (deltay * (resY-1)) / deltabb;
}

inline int Grid::getIndexOfCoordinateZ(double z) const {
    double deltabb = bb.getMaxZ() - bb.getMinZ();
    double deltaz = z - bb.getMinZ();
    return (deltaz * (resZ-1)) / deltabb;
}

#endif // GRID_H
