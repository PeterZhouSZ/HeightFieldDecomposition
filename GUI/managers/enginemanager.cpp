#include "enginemanager.h"
#include "ui_enginemanager.h"
#include "common.h"
#include <cstdio>
#include <QFileDialog>
#include <omp.h>

EngineManager::EngineManager(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::EngineManager),
    mainWindow((MainWindow*)parent),
    g(nullptr),
    d(nullptr),
    b(nullptr),
    iterations(nullptr),
    solutions(nullptr){
    ui->setupUi(this);
    ui->iterationsSlider->setMaximum(0);
}

void EngineManager::deleteDrawableObject(DrawableObject* d) {
    if (d != nullptr){
        d->setVisible(false);
        mainWindow->deleteObj(d);
        delete d;
        d = nullptr;
    }
}

EngineManager::~EngineManager() {
    delete ui;
}

void EngineManager::updateLabel(double value, QLabel* label) {
    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::digits10+1);
    ss << value;
    label->setText(QString::fromStdString(ss.str()));
}

void EngineManager::serialize(std::ofstream& binaryFile) const {
    g->serialize(binaryFile);
    d->serialize(binaryFile);
    b->serialize(binaryFile);
    bool b = false;
    if (solutions!=nullptr){
        b = true;
        Serializer::serialize(b, binaryFile);
        solutions->serialize(binaryFile);
    }
    else
        Serializer::serialize(b, binaryFile);
}

void EngineManager::deserialize(std::ifstream& binaryFile) {
    deleteDrawableObject(g);
    deleteDrawableObject(d);
    deleteDrawableObject(b);
    g = new DrawableGrid();
    d = new DrawableDcel();
    b = new Box3D();
    g->deserialize(binaryFile);
    d->deserialize(binaryFile);
    b->deserialize(binaryFile);
    d->update();
    mainWindow->pushObj(d, "Scaled Mesh");
    mainWindow->pushObj(g, "Grid");
    mainWindow->pushObj(b, "Box");
    e = Energy(*g);
    ui->wSpinBox->setValue(b->getMax().x()-b->getMin().x());
    ui->hSpinBox->setValue(b->getMax().y()-b->getMin().y());
    ui->dSpinBox->setValue(b->getMax().z()-b->getMin().z());
    ui->weigthsRadioButton->setChecked(true);
    ui->sliceCheckBox->setChecked(true);
    g->setDrawBorders();
    g->setSlice(1);

    bool b;
    Serializer::deserialize(b, binaryFile);
    if (b){
        deleteDrawableObject(solutions);
        solutions = new BoxList();
        solutions->deserialize(binaryFile);
        solutions->setVisibleBox(0);
        solutions->setCylinders(false);
        mainWindow->pushObj(solutions, "Solutions");
        ui->showAllSolutionsCheckBox->setEnabled(true);
        ui->solutionsSlider->setEnabled(true);

    }

    mainWindow->updateGlCanvas();
}

void EngineManager::on_generateGridPushButton_clicked() {
    DcelManager* dm = (DcelManager*)mainWindow->getManager(DCEL_MANAGER_ID);
    DrawableDcel* dd = dm->getDcel();
    if (dd != nullptr){
        deleteDrawableObject(d);
        d = new DrawableDcel(*dd);
        mainWindow->pushObj(d, "Scaled Mesh");
        deleteDrawableObject(g);
        g = new DrawableGrid();

        Engine::generateGrid(*g, *d, ui->samplesSpinBox->value(), XYZ[ui->targetComboBox->currentIndex()], ui->distanceSpinBox->value());

        d->update();
        g->setKernelDistance(ui->distanceSpinBox->value());
        e = Energy(*g);
        mainWindow->pushObj(g, "Grid");
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_distanceSpinBox_valueChanged(double arg1) {
    if (g!=nullptr){
        g->setKernelDistance(arg1);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_targetComboBox_currentIndexChanged(int index) {
    if (d!= nullptr && g!= nullptr){
        g->setTarget(XYZ[index]);
        g->calculateBorderWeights(*d);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_kernelRadioButton_toggled(bool checked) {
    if (checked && g!=nullptr){
        g->setDrawKernel();
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_weigthsRadioButton_toggled(bool checked) {
    if (checked && g!=nullptr){
        g->setDrawBorders();
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_freezeKernelPushButton_clicked() {
    if (g!=nullptr && d!=nullptr){
        double value = ui->distanceSpinBox->value();
        g->calculateWeightsAndFreezeKernel(*d, value);
        deleteDrawableObject(b);
        Pointd p1(0,0,0);
        Pointd p2(ui->wSpinBox->value(), ui->hSpinBox->value(), ui->dSpinBox->value());
        b = new Box3D(p1, p2, QColor(0,0,0));
        mainWindow->pushObj(b, "Box");
        e = Energy(*g);
        mainWindow->updateGlCanvas();
    }

}

void EngineManager::on_sliceCheckBox_stateChanged(int arg1) {
    if (g!=nullptr){
        if (arg1 == Qt::Checked){
            ui->sliceComboBox->setEnabled(true);
            ui->sliceSlider->setEnabled(true);
            int s = ui->sliceComboBox->currentIndex();
            g->setSliceValue(0);
            g->setSlice(s+1);
            if (s == 0) ui->sliceSlider->setMaximum(g->getResX() -1);
            if (s == 1) ui->sliceSlider->setMaximum(g->getResY() -1);
            if (s == 2) ui->sliceSlider->setMaximum(g->getResZ() -1);
        }
        else {
            ui->sliceComboBox->setEnabled(false);
            ui->sliceSlider->setEnabled(false);
            g->setSlice(0);
        }
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_sliceSlider_valueChanged(int value) {
    if (g!=nullptr){
        g->setSliceValue(value);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_sliceComboBox_currentIndexChanged(int index) {
    if (g!=nullptr){
        g->setSliceValue(0);
        g->setSlice(index+1);
        ui->sliceSlider->setValue(0);
        if (index == 0) ui->sliceSlider->setMaximum(g->getResX() -1);
        if (index == 1) ui->sliceSlider->setMaximum(g->getResY() -1);
        if (index == 2) ui->sliceSlider->setMaximum(g->getResZ() -1);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_serializePushButton_clicked() {
    QString filename = QFileDialog::getSaveFileName(nullptr,
                       "Serialize",
                       ".",
                       "BIN(*.bin)");
    if (!filename.isEmpty()) {
        std::ofstream myfile;
        myfile.open (filename.toStdString(), std::ios::out | std::ios::binary);
        serialize(myfile);
        myfile.close();
    }
}

void EngineManager::on_deserializePushButton_clicked() {
    QString filename = QFileDialog::getOpenFileName(nullptr,
                       "Deserialize",
                       ".",
                       "BIN(*.bin)");

    if (!filename.isEmpty()) {
        std::ifstream myfile;
        myfile.open (filename.toStdString(), std::ios::in | std::ios::binary);
        deserialize(myfile);
        myfile.close();
    }
}

void EngineManager::on_wSpinBox_valueChanged(double arg1) {
    if (b!=nullptr){
        b->setW(arg1);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_hSpinBox_valueChanged(double arg1) {
    if (b!=nullptr){
        b->setH(arg1);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_dSpinBox_valueChanged(double arg1) {
    if (b!=nullptr){
        b->setD(arg1);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_plusXButton_clicked() {
    if (b!=nullptr){
        if (ui->boxRadioButton->isChecked())
            b->moveX(ui->stepSpinBox->value());
        else {
            Pointd c;
            if (ui->c1RadioButton->isChecked()){
                c = b->getConstraint1();
                b->setConstraint1(Pointd(c.x()+ui->stepSpinBox->value(), c.y(), c.z()));
            }
            if (ui->c2RadioButton->isChecked()){
                c = b->getConstraint2();
                b->setConstraint2(Pointd(c.x()+ui->stepSpinBox->value(), c.y(), c.z()));
            }
            if (ui->c3RadioButton->isChecked()){
                c = b->getConstraint3();
                b->setConstraint3(Pointd(c.x()+ui->stepSpinBox->value(), c.y(), c.z()));
            }
        }
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_minusXButton_clicked() {
    if (b!=nullptr){
        if (ui->boxRadioButton->isChecked())
            b->moveX(- ui->stepSpinBox->value());
        else {
            Pointd c;
            if (ui->c1RadioButton->isChecked()){
                c = b->getConstraint1();
                b->setConstraint1(Pointd(c.x()-ui->stepSpinBox->value(), c.y(), c.z()));
            }
            if (ui->c2RadioButton->isChecked()){
                c = b->getConstraint2();
                b->setConstraint2(Pointd(c.x()-ui->stepSpinBox->value(), c.y(), c.z()));
            }
            if (ui->c3RadioButton->isChecked()){
                c = b->getConstraint3();
                b->setConstraint3(Pointd(c.x()-ui->stepSpinBox->value(), c.y(), c.z()));
            }
        }
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_plusYButton_clicked() {
    if (b!=nullptr){
        if (ui->boxRadioButton->isChecked())
            b->moveY(ui->stepSpinBox->value());
        else {
            Pointd c;
            if (ui->c1RadioButton->isChecked()){
                c = b->getConstraint1();
                b->setConstraint1(Pointd(c.x(), c.y()+ui->stepSpinBox->value(), c.z()));
            }
            if (ui->c2RadioButton->isChecked()){
                c = b->getConstraint2();
                b->setConstraint2(Pointd(c.x(), c.y()+ui->stepSpinBox->value(), c.z()));
            }
            if (ui->c3RadioButton->isChecked()){
                c = b->getConstraint3();
                b->setConstraint3(Pointd(c.x(), c.y()+ui->stepSpinBox->value(), c.z()));
            }

        }
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_minusYButton_clicked() {
    if (b!=nullptr){
        if (ui->boxRadioButton->isChecked())
            b->moveY(- ui->stepSpinBox->value());
        else {
            Pointd c;
            if (ui->c1RadioButton->isChecked()){
                c = b->getConstraint1();
                b->setConstraint1(Pointd(c.x(), c.y()-ui->stepSpinBox->value(), c.z()));
            }
            if (ui->c2RadioButton->isChecked()){
                c = b->getConstraint2();
                b->setConstraint2(Pointd(c.x(), c.y()-ui->stepSpinBox->value(), c.z()));
            }
            if (ui->c3RadioButton->isChecked()){
                c = b->getConstraint3();
                b->setConstraint3(Pointd(c.x(), c.y()-ui->stepSpinBox->value(), c.z()));
            }

        }
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_plusZButton_clicked() {
    if (b!=nullptr){
        if (ui->boxRadioButton->isChecked())
            b->moveZ(ui->stepSpinBox->value());
        else {
            Pointd c;
            if (ui->c1RadioButton->isChecked()){
                c = b->getConstraint1();
                b->setConstraint1(Pointd(c.x(), c.y(), c.z()+ui->stepSpinBox->value()));
            }
            if (ui->c2RadioButton->isChecked()){
                c = b->getConstraint2();
                b->setConstraint2(Pointd(c.x(), c.y(), c.z()+ui->stepSpinBox->value()));
            }
            if (ui->c3RadioButton->isChecked()){
                c = b->getConstraint3();
                b->setConstraint3(Pointd(c.x(), c.y(), c.z()+ui->stepSpinBox->value()));
            }

        }
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_minusZButton_clicked() {
    if (b!=nullptr){
        if (ui->boxRadioButton->isChecked())
            b->moveZ(- ui->stepSpinBox->value());
        else {
            Pointd c;
            if (ui->c1RadioButton->isChecked()){
                c = b->getConstraint1();
                b->setConstraint1(Pointd(c.x(), c.y(), c.z()-ui->stepSpinBox->value()));
            }
            if (ui->c2RadioButton->isChecked()){
                c = b->getConstraint2();
                b->setConstraint2(Pointd(c.x(), c.y(), c.z()-ui->stepSpinBox->value()));
            }
            if (ui->c3RadioButton->isChecked()){
                c = b->getConstraint3();
                b->setConstraint3(Pointd(c.x(), c.y(), c.z()-ui->stepSpinBox->value()));
            }

        }
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_energyBoxPushButton_clicked() {
    if (b!=nullptr){
        double energy = e.energy(*b);
        Eigen::VectorXd gradient(6);
        e.gradientTricubicInterpolationEnergy(gradient, b->getMin(), b->getMax());
        updateLabel(gradient(0), ui->gminx);
        updateLabel(gradient(1), ui->gminy);
        updateLabel(gradient(2), ui->gminz);
        updateLabel(gradient(3), ui->gmaxx);
        updateLabel(gradient(4), ui->gmaxy);
        updateLabel(gradient(5), ui->gmaxz);
        std::cerr << "\nGradient: \n" << gradient << "\n";
        updateLabel(energy, ui->energyLabel);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_minimizePushButton_clicked() {
    if (b!=nullptr){
        double it;
        if (ui->saveIterationsCheckBox->isChecked()){
            deleteDrawableObject(iterations);
            iterations = new BoxList();
            Timer t("Gradient Discend");
            it = e.gradientDiscend(*b, *iterations);
            t.stopAndPrint();
            iterations->setVisibleBox(0);
            ui->iterationsSlider->setMaximum(iterations->getNumberBoxes()-1);
            mainWindow->pushObj(iterations, "Iterations");

            double energy = e.energy(iterations->getBox(0));
            updateLabel(energy, ui->energyIterationLabel);
        }
        else {
            Timer t("Gradient Discend");
            it = e.gradientDiscend(*b);
            t.stopAndPrint();
        }
        updateLabel(it, ui->minimizedEnergyLabel);
        double energy = e.energy(*b);
        updateLabel(energy, ui->energyLabel);
        ui->wSpinBox->setValue(b->getMax().x()-b->getMin().x());
        ui->hSpinBox->setValue(b->getMax().y()-b->getMin().y());
        ui->dSpinBox->setValue(b->getMax().z()-b->getMin().z());
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_serializeBoxPushButton_clicked() {
    if (b!=nullptr){
        std::ofstream myfile;
        myfile.open ("box.bin", std::ios::out | std::ios::binary);
        b->serialize(myfile);
        myfile.close();
    }
}

void EngineManager::on_deserializeBoxPushButton_clicked() {
    deleteDrawableObject(b);
    b = new Box3D();
    mainWindow->pushObj(b, "Box");
    std::ifstream myfile;
    myfile.open ("box.bin", std::ios::in | std::ios::binary);
    b->deserialize(myfile);
    myfile.close();
    mainWindow->updateGlCanvas();
}

void EngineManager::on_iterationsSlider_sliderMoved(int position) {
    if (iterations != nullptr){
        iterations->setVisibleBox(position);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_energyIterationsButton_clicked() {
    if (iterations != nullptr){
        Box3D b = iterations->getBox(ui->iterationsSlider->value());
        double energy = e.energy(b);
        Eigen::VectorXd gradient(6);
        Eigen::VectorXd finiteGradient(6);
        e.gradientTricubicInterpolationEnergy(gradient, b.getMin(), b.getMax());
        e.gradientEnergyFiniteDifference(finiteGradient, b);
        std::cerr << "Gradient: \n" << gradient << "\n";
        std::cerr << "Finite Gradient: \n" << finiteGradient << "\n";
        updateLabel(energy, ui->energyIterationLabel);
    }
}

void EngineManager::on_createBoxesPushButton_clicked() {
    if (d!=nullptr){
        deleteDrawableObject(solutions);
        solutions = new BoxList();
        //Engine::calculateInitialBoxes(*solutions, *d, Eigen::Matrix3d::Identity(), true, XYZ[ui->targetComboBox->currentIndex()]);
        Engine::calculateInitialBoxes(*solutions, *d);
        ui->showAllSolutionsCheckBox->setEnabled(true);
        solutions->setVisibleBox(0);
        ui->solutionsSlider->setMaximum(solutions->getNumberBoxes()-1);
        mainWindow->pushObj(solutions, "Solutions");
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_showAllSolutionsCheckBox_stateChanged(int arg1) {
    if (arg1 == Qt::Checked){
        ui->solutionsSlider->setEnabled(false);
        ui->solutionsSlider->setValue(0);
        solutions->setCylinders(true);
        solutions->setVisibleBox(-1);
    }
    else {
        ui->solutionsSlider->setEnabled(true);
        ui->solutionsSlider->setValue(0);
        solutions->setCylinders(false);
        solutions->setVisibleBox(0);
    }
    mainWindow->updateGlCanvas();
}

void EngineManager::on_solutionsSlider_valueChanged(int value) {
    if (ui->solutionsSlider->isEnabled()){
        solutions->setVisibleBox(value);
        mainWindow->updateGlCanvas();
    }
}

void EngineManager::on_minimizeAllPushButton_clicked() {
    if (solutions != nullptr){
        Timer total("Minimization All Boxes");
        int np = solutions->getNumberBoxes();
        # pragma omp parallel for if(np>10)
        for (int i = 0; i < np; i++){
            Box3D b = solutions->getBox(i);
            std::stringstream ss;
            ss << "Minimization " << i << " box";
            Timer t(ss.str());
            e.gradientDiscend(b);
            t.stopAndPrint();
            //solutions->setBox(i, b);
            //solutions->setVisibleBox(i);
            //mainWindow->updateGlCanvas();
            std::cerr << "Total: " << total.delay() << "\n\n";
        }
        total.stopAndPrint();
    }

}
