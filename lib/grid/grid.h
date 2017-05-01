#ifndef GRID_H
#define GRID_H

#include "common/bounding_box.h"
#include "common/arrays.h"
#include "dcel/dcel.h"
#include "engine/tricubic.h"
#include "common.h"

#include "cgal/aabbtree.h"

class Grid : public SerializableObject{
    public:

        Grid();
        Grid(const Pointi& resolution, const Array3D<Pointd>& gridCoordinates, const Array3D<gridreal>& signedDistances, const Pointd& gMin, const Pointd& gMax);

        unsigned int getResX() const;
        unsigned int getResY() const;
        unsigned int getResZ() const;

        BoundingBox getBoundingBox() const;


        Vec3 getTarget() const;
        void setTarget(const Vec3& value);

        void calculateBorderWeights(const Dcel &d, bool tolerance = false, std::set<const Dcel::Face*>& savedFaces = Grid::dummy);
        void calculateWeightsAndFreezeKernel(const Dcel& d, double value, bool tolerance = false, std::set<const Dcel::Face*>& savedFaces = Grid::dummy);
        void calculateFullBoxValues(double (*integralTricubicInterpolation)(const gridreal *&, double, double, double, double, double, double));

        double getValue(const Pointd &p) const;
        double getUnit() const;
        void getMinAndMax(double &min, double &max);

        Pointd getNearestGridPoint(const Pointd& p) const;
        void getCoefficients(const gridreal*& coeffs, const Pointd& p) const;
        double getFullBoxValue(const Pointd&p) const;

        // SerializableObject interface
        void serialize(std::ofstream& binaryFile) const;
        bool deserialize(std::ifstream& binaryFile);

        void resetSignedDistances();


    protected:
        Pointd getPoint(unsigned int i, unsigned int j, unsigned int k) const;
        unsigned int getIndex(unsigned int i, unsigned int j, unsigned int k) const;
        double getSignedDistance(unsigned int i, unsigned int j, unsigned int k) const;
        double getWeight(unsigned int i, unsigned int j, unsigned int k) const;
        int getIndexOfCoordinateX(double x) const;
        int getIndexOfCoordinateY(double y) const;
        int getIndexOfCoordinateZ(double z) const;

        void getCoefficients(const gridreal*& coeffs, unsigned int i, unsigned int j, unsigned int k) const;

        void setWeightOnCube(unsigned int i, unsigned int j, unsigned int k, double w);

        BoundingBox bb;
        unsigned int resX, resY, resZ;
        Array3D<gridreal> signedDistances;
        Array3D<gridreal> weights;
        //Array4D<gridreal> coeffs;
        std::vector< std::array<gridreal, 64> > coeffs;
        Array3D<int> mapCoeffs;
        Array3D<gridreal> fullBoxValues;
        Vec3 target;
        double unit;

        static std::set<const Dcel::Face*> dummy;

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
    return unit;
}

inline Pointd Grid::getNearestGridPoint(const Pointd& p) const{
    return Pointd(bb.getMinX() + getIndexOfCoordinateX(p.x())*unit, bb.getMinY() + getIndexOfCoordinateY(p.y())*unit, bb.getMinZ() + getIndexOfCoordinateZ(p.z())*unit);
}

inline void Grid::getCoefficients(const gridreal* &coeffs, unsigned int i, unsigned int j, unsigned int k) const {
    int id = mapCoeffs(i,j,k);
    coeffs = this->coeffs[id].data();
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

inline void Grid::getCoefficients(const gridreal*& coeffs, const Pointd& p) const {
    if(bb.isStrictlyIntern(p)){
        int id = mapCoeffs(getIndexOfCoordinateX(p.x()), getIndexOfCoordinateY(p.y()), getIndexOfCoordinateZ(p.z()));
        coeffs = this->coeffs[id].data();
    }
    else coeffs = this->coeffs[0].data();
}

inline double Grid::getFullBoxValue(const Pointd& p) const {
    if(bb.isStrictlyIntern(p))
        return fullBoxValues(getIndexOfCoordinateX(p.x()), getIndexOfCoordinateY(p.y()), getIndexOfCoordinateZ(p.z()));
    else return fullBoxValues(0,0,0);
}

inline void Grid::resetSignedDistances() {
    signedDistances.resize(0,0,0);
}

inline Pointd Grid::getPoint(unsigned int i, unsigned int j, unsigned int k) const {
    return Pointd(bb.getMinX() + i*unit, bb.getMinY() + j*unit, bb.getMinZ() + k*unit);
}

inline unsigned int Grid::getIndex(unsigned int i, unsigned int j, unsigned int k) const {
    assert (i < resX);
    assert (j < resY);
    assert (k < resZ);
    return k+resZ*(j + resY*i);
}

inline double Grid::getSignedDistance(unsigned int i, unsigned int j, unsigned int k) const {
    return signedDistances(i,j,k);
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
