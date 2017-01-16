#include "heightfieldslist.h"
#include "common.h"

HeightfieldsList::HeightfieldsList() : visible(true), nVisible(-1) {
}

void HeightfieldsList::draw() const {
    if (visible){
        if (nVisible < 0){
            for (unsigned int i = 0; i < heightfields.size(); ++i){
                heightfields[i].draw();
            }
        }
        else {
            heightfields[nVisible].draw();
        }
    }
}

Pointd HeightfieldsList::sceneCenter() const{
    return std::move(Pointd());
}

double HeightfieldsList::sceneRadius() const {
    return -1;
}

bool HeightfieldsList::isVisible() const {
    return visible;
}

void HeightfieldsList::setVisible(bool b) {
    visible = b;
}

void HeightfieldsList::setVisibleHeightfield(int i) {
    assert (i < (int)heightfields.size());
    nVisible = i;
}

void HeightfieldsList::resize(int n) {
    heightfields.resize(n);
    targets.resize(n);
}

unsigned int HeightfieldsList::getNumberVerticesHeightfield(int i) const {
    assert (i < (int)heightfields.size());
    return heightfields[i].getNumberVertices();
}

Pointd HeightfieldsList::getVertexOfHeightfield(int he, int v) const {
    assert (he < (int)heightfields.size());
    return heightfields[he].getVertex(v);
}

Vec3 HeightfieldsList::getTarget(int i) const {
    assert(i < (int)heightfields.size());
    return targets[i];
}

void HeightfieldsList::setWireframe(bool b) {
    for (unsigned int i = 0; i < heightfields.size(); ++i){
        heightfields[i].setWireframe(b);
    }
}

void HeightfieldsList::setPointShading() {
    for (unsigned int i = 0; i < heightfields.size(); ++i){
        heightfields[i].setPointsShading();
    }
}

void HeightfieldsList::setFlatShading() {
    for (unsigned int i = 0; i < heightfields.size(); ++i){
        heightfields[i].setFlatShading();
    }
}

void HeightfieldsList::setSmoothShading() {
    for (unsigned int i = 0; i < heightfields.size(); ++i){
        heightfields[i].setSmoothShading();
    }
}

void HeightfieldsList::checkHeightfields() const {
    for (unsigned int i = 0; i < heightfields.size(); i++){
        const IGLInterface::DrawableIGLMesh& m = heightfields[i];
        for (unsigned int f = 0; f < m.getNumberFaces(); f++){
            if (m.getFaceNormal(f).dot(targets[i]) < FLIP_ANGLE-EPSILON && m.getFaceNormal(f).dot(targets[i]) > -1 + EPSILON){
                std::cerr << "Hieghtfield: " << i << "; Triangle: " << f << "; Flip: " << m.getFaceNormal(f).dot(targets[i]) << "\n";
            }
        }
    }
}

void HeightfieldsList::rotate(const Eigen::MatrixXd m) {
    for (unsigned int i = 0; i < heightfields.size(); i++){
        heightfields[i].rotate(m);
        heightfields[i].updateVertexNormals();
    }
}

void HeightfieldsList::addHeightfield(const IGLInterface::DrawableIGLMesh& m, const Vec3& target, int i) {
    if (i < 0){
        heightfields.push_back(m);
        targets.push_back(target);
        QColor c = colorOfNormal(target);
        heightfields[heightfields.size()-1].setFaceColor(c.redF(), c.greenF(), c.blueF());
        for (unsigned int i = 0; i < m.getNumberFaces(); i++){
            if (m.getFaceNormal(i).dot(target) < FLIP_ANGLE-EPSILON)
                heightfields[heightfields.size()-1].setFaceColor(0,0,0,i);
        }
    }
    else {
        heightfields[i] = m;
        targets[i] = target;
        QColor c = colorOfNormal(target);
        heightfields[i].setFaceColor(c.redF(), c.greenF(), c.blueF());
        for (unsigned int j = 0; j < m.getNumberFaces(); j++){
            if (m.getFaceNormal(j).dot(target) < FLIP_ANGLE-EPSILON)
                heightfields[i].setFaceColor(0,0,0,j);
        }
    }
}

unsigned int HeightfieldsList::getNumHeightfields() const {
    return heightfields.size();
}

void HeightfieldsList::removeHeightfield(unsigned int i) {
    assert (i < heightfields.size());
    heightfields.erase(heightfields.begin()+i);
    targets.erase(targets.begin()+i);
}

const IGLInterface::IGLMesh& HeightfieldsList::getHeightfield(unsigned int i) const {
    assert (i < heightfields.size());
    return heightfields[i];
}

IGLInterface::IGLMesh& HeightfieldsList::getHeightfield(unsigned int i) {
    assert (i < heightfields.size());
    return heightfields[i];
}

/*IGLInterface::IGLMesh HeightfieldsList::getHeightfield(unsigned int i) const {
    assert (i < heightfields.size());
    return heightfields[i];
}*/

void HeightfieldsList::setHeightfield(const IGLInterface::IGLMesh& m, unsigned int i, bool updateColor) {
    assert (i < heightfields.size());
    heightfields[i] = m;
    if (updateColor){
        QColor c = colorOfNormal(targets[i]);
        heightfields[i].setFaceColor(c.redF(), c.greenF(), c.blueF());
        for (unsigned int j = 0; j < m.getNumberFaces(); j++){
            if (m.getFaceNormal(j).dot(targets[i]) < FLIP_ANGLE-EPSILON)
                heightfields[i].setFaceColor(0,0,0,j);
        }
    }
}

void HeightfieldsList::insertHeightfield(const IGLInterface::IGLMesh& m, const Vec3& target, unsigned int i) {
    assert (i < heightfields.size()+1);
    heightfields.insert(heightfields.begin() + i, m);
    targets.insert(targets.begin() + i, target);
    QColor c = colorOfNormal(target);
    heightfields[i].setFaceColor(c.redF(), c.greenF(), c.blueF());
    for (unsigned int j = 0; j < m.getNumberFaces(); j++){
        if (m.getFaceNormal(j).dot(target) < FLIP_ANGLE-EPSILON)
            heightfields[i].setFaceColor(0,0,0,j);
    }
}

void HeightfieldsList::explode(const Pointd& bc, double dist) {
    for (unsigned int i = 0; i < heightfields.size(); ++i) {
        Pointd translation;
        Vec3 v = heightfields[i].getBarycenter() - bc;
        v.normalize();
        translation = v * dist;
        heightfields[i].translate(translation);
    }
}

void HeightfieldsList::serialize(std::ofstream& binaryFile) const {
    Serializer::serialize(heightfields, binaryFile);
    Serializer::serialize(targets, binaryFile);
}

bool HeightfieldsList::deserialize(std::ifstream& binaryFile) {
    std::vector<IGLInterface::DrawableIGLMesh> tmp;
    if (Serializer::deserialize(tmp, binaryFile) &&
            Serializer::deserialize(targets, binaryFile)){
        heightfields = std::move(tmp);
        nVisible = -1;
        return true;
    }
    else
        return false;

}
