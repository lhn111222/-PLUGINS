#include "qVolumeMeasureDlg.h"

qVolumeMeasureDlg::qVolumeMeasureDlg(ccMainAppInterface* app)
    : QDialog(app ? app->getMainWindow() : nullptr,  Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint)
    , m_app(app)
    , m_glWindow(nullptr)
    , m_rasterMesh(nullptr)
    , m_cloud()
    , m_label(cc2DLabel("Ground"))
    , m_pointsIdx(std::vector<unsigned>())
    , currentPath(QDir("./"))
    , gridStep(0.5)
    , gridWidth(0)
    , gridHeight(0)
{
    setupUi(this);
    setModal(true);

    {
        QWidget* glWidget = nullptr;
        m_app->createGLWindow(m_glWindow, glWidget);
        assert(m_glWindow && glWidget);

        m_glWindow->setPerspectiveState(false, true);
        // 新版修补参数
        m_glWindow->displayOverlayEntities(true,true);
        m_glWindow->setInteractionMode(ccGLWindow::MODE_TRANSFORM_CAMERA);
        m_glWindow->setPickingMode(ccGLWindow::POINT_PICKING);

        //add window to the input frame (if any)
        viewFrame->setLayout(new QHBoxLayout());
        viewFrame->layout()->addWidget(glWidget);
    }

    {
        connect(clearPushButton, &QPushButton::clicked, this, &qVolumeMeasureDlg::onClearPushButtonClick);
        connect(okPushButton, &QPushButton::clicked, this, &qVolumeMeasureDlg::onOkPushButtonClick);
        connect(calPushButton, &QPushButton::clicked, this, &qVolumeMeasureDlg::onCalPushButtonClick);
        connect(genReportPushButton, &QPushButton::clicked, this, &qVolumeMeasureDlg::onGenReportPushButtonClick);
        connect(switchPcMeshPushButton, &QPushButton::clicked, this, &qVolumeMeasureDlg::onSwitchPushButtonClick);
        connect(densityLineEdit, &QLineEdit::textChanged, this, [&](const QString& s) {report.density = s.toDouble();});
        //ccGLWindow信号改变
        // connect(m_glWindow, &ccGLWindow::itemPickedFast, this, &qVolumeMeasureDlg::handlePickedItem);
        m_glWindow->setPickingMode(ccGLWindow::POINT_PICKING);
        connect(m_glWindow->signalEmitter(), &ccGLWindowSignalEmitter::itemPicked, this, &qVolumeMeasureDlg::handlePickedItem);
    } // 修正：补充闭合之前的代码块

    {
        okPushButton->setEnabled(false);
        clearPushButton->setEnabled(false);
        calPushButton->setEnabled(false);
        genReportPushButton->setEnabled(false);
        switchPcMeshPushButton->setEnabled(false);
    }

    {
        m_glWindow->addToOwnDB(&m_label);
        m_label.setVisible(true);
        m_label.setDisplayedIn2D(false);
    }

    {
        report.volumeAbove = 0;
        report.volumeUnder = 0;
        report.density = 0;
    }
}

qVolumeMeasureDlg::~qVolumeMeasureDlg()
{
    if (m_glWindow)
    {
        m_glWindow->getOwnDB()->removeAllChildren();
        if (m_app)
        {
            m_app->destroyGLWindow(m_glWindow);
            m_glWindow = nullptr;
        }
    }

    if (m_cloud.originDisplay)
        static_cast<ccGLWindow*>(m_cloud.originDisplay)->zoomGlobal();
}

bool qVolumeMeasureDlg::setCloud(ccPointCloud* cloud)
{
    m_cloud.backup(cloud);
    if (!m_cloud.backupColors())
    {
        //failed to backup the cloud colors
        QMessageBox::warning(this,
                             QString::fromUtf8("警告"),
                             QString::fromUtf8("运行内存不足"));
        return false;
    }

    if (!cloud->getOctree())
    {
        ccProgressDialog pDlg(true, this);
        ccOctree::Shared octree = cloud->computeOctree(&pDlg);
        if (!octree)
        {
            QMessageBox::warning(this,
                                 QString::fromUtf8("警告"),
                                 QString::fromUtf8("运行内存不足"));
            return false;
        }
    }


    ccRasterGrid::ComputeGridSize(2, cloud->getOwnBB(), gridStep, gridWidth, gridHeight);
    m_glWindow->addToOwnDB(cloud);
    m_glWindow->zoomGlobal();
    m_glWindow->setView(CC_TOP_VIEW);
    m_glWindow->redraw();


    QMessageBox::information(this
                             , QString::fromUtf8("Friendly Reminder")
                             , QString::fromUtf8("Please select three points on the ground (left-click on the point cloud) to help the software determine the ground location. Click the confirm button to save after successful selection."));
    return true;
}

void qVolumeMeasureDlg::onClearPushButtonClick()
{
    if (m_rasterMesh) {
        m_rasterMesh->removeAllChildren();
        m_glWindow->removeFromOwnDB(m_rasterMesh);
        m_rasterMesh = nullptr;
    }

    clearPushButton->setEnabled(false);
    okPushButton->setEnabled(false);
    calPushButton->setEnabled(false);
    genReportPushButton->setEnabled(false);
    switchPcMeshPushButton->setEnabled(false);

    m_label.clear();
    m_label.setVisible(true);
    m_cloud.ref->setEnabled(true);
    m_pointsIdx.clear();
    m_glWindow->redraw();
}

void qVolumeMeasureDlg::onCalPushButtonClick()
{
    calPushButton->setEnabled(false);

    // gen progress dlg
    auto pDlg = std::make_unique<ccProgressDialog>(false, this);
    {
        pDlg->setMethodTitle(QString::fromUtf8("calculate volume......"));
        pDlg->start();
        pDlg->show();
        QCoreApplication::processEvents();
    }
    CCCoreLib::NormalizedProgress nProgress(pDlg.get(), 3+gridHeight*gridWidth, 3+gridHeight*gridWidth);

    // get ground height
    pDlg->setInfo(QString::fromUtf8("Calculate ground elevation..."));
    qreal groundHeight = 0;
    ccGLMatrix rotateMatrix;
    {
        auto zAix = CCVector3(0, 0, 1);
        auto line1 = *(m_cloud.ref->getPoint(m_pointsIdx[0])) - *(m_cloud.ref->getPoint(m_pointsIdx[1]));
        auto line2 = *(m_cloud.ref->getPoint(m_pointsIdx[0])) - *(m_cloud.ref->getPoint(m_pointsIdx[2]));
        auto directon = line1.cross(line2);
        directon.normalize();
        if (directon.angle_rad(zAix) > 1.57) // 如果 direction 的方向是反的
            directon *= -1; // 给他翻转过来
        rotateMatrix = ccGLMatrix::FromToRotation(directon,zAix);
        m_cloud.ref->rotateGL(rotateMatrix);
        m_cloud.ref->applyGLTransformation_recursive();

        // 重新计算旋转后的地面高度，这次三个点的高度是相同的
        groundHeight = m_cloud.ref->getPoint(m_pointsIdx.front())->z;
    }
    nProgress.oneStep();

    // init ceil raster
    pDlg->setInfo(QString::fromUtf8("初始化栅格..."));
    ccRasterGrid ceilRaster;
    {
        auto box = m_cloud.ref->getOwnBB();
        auto minConer = CCVector3d::fromArray(box.minCorner().u);

        auto gridTotalSize = gridWidth * gridHeight;
        if (gridTotalSize == 1)
        {
            dispToConsole(QString::fromUtf8("点云体积太小，针对当前点云的计算结果误差较大"));
        }
        else if (gridTotalSize > 10000000)
        {
            dispToConsole(QString::fromUtf8("点云体积太大，针对当前点云的计算用时较长"));
        }
        if (!ceilRaster.init(gridWidth, gridHeight, gridStep, minConer))
        {
            QMessageBox::warning(this,
                                 QString::fromUtf8("警告"),
                                 QString::fromUtf8("运行内存不足"));
            assert(false);
        }
        ccRasterGrid::DelaunayInterpolationParams delaunayParams; // 定义插值参数结构体
        delaunayParams.maxEdgeLength = 0.2; // 设置最大边长度（可调整）
        if (!ceilRaster.fillWith(
                m_cloud.ref,                              // 1. 点云
                2,                                        // 2. 投影轴（Z轴）
                ccRasterGrid::ProjectionType::PROJ_AVERAGE_VALUE, // 3. 投影类型
                ccRasterGrid::InterpolationType::DELAUNAY,       // 4. 空单元格插值方式（DELAUNAY）
                &delaunayParams,                                 // 5. 插值参数（结构体指针）
                ccRasterGrid::INVALID_PROJECTION_TYPE,          // 6. 标量场投影类型
                pDlg.get(),                                     // 7. 进度条
                -1                                             // 8. 高度标准差标量场索引
                ))
        {
            QMessageBox::warning(this,
                                 QString::fromUtf8("警告"),
                                 QString::fromUtf8("运行内存不足"));
            assert(false);
        }
        //新版没这个定义
        //ceilRaster.fillEmptyCells(ccRasterGrid::ExportedScalarField::PER_CELL_HEIGHT_FIELD);

        ceilRaster.fillEmptyCells(ccRasterGrid::EmptyCellFillOption::INTERPOLATE_DELAUNAY);
        dispToConsole(
            QString::fromUtf8("栅格规模: %1 x %2")
                .arg(ceilRaster.width).arg(ceilRaster.height)
            );
    }
    nProgress.oneStep();

    pDlg->setInfo(QString::fromUtf8("计算体积，单元数量: %1 x %2").arg(gridWidth).arg(gridHeight));
    {
        qreal& volumeAbove = report.volumeAbove = 0;
        qreal& volumeUnder = report.volumeUnder = 0;
        for (size_t i = 0; i < gridHeight; i++)
        {
            for (size_t j = 0; j < gridWidth; j++)
            {
                if (std::isfinite(ceilRaster.rows[i][j].h))
                {
                    auto h = ceilRaster.rows[i][j].h - groundHeight;
                    if (h > 0)
                        volumeAbove += h;
                    else
                        volumeUnder -= h;
                }
            }
            nProgress.oneStep();
        }

        volumeAbove *= static_cast<qreal>(gridStep * gridStep);
        volumeUnder *= static_cast<qreal>(gridStep * gridStep);

        dispToConsole(QString::fromUtf8(
                          "\n======================"
                          "\n= 地上体积：%1 m³"
                          "\n= 地下体积：%2 m³"
                          "\n======================\n"
                          ).arg(volumeAbove, 0, 'f', 3).arg(volumeUnder, 0, 'f', 3));
    }

    pDlg->setInfo(QString::fromUtf8("生成 mesh ..."));
    {
        /* auto rasterCloud = ceilRaster.convertToCloud({ ccRasterGrid::ExportedScalarField::PER_CELL_HEIGHT_FIELD },
                                                     false, // interpolate scalar field
                                                     false, // interpolate colors
                                                     false, // resample input cloud xy
                                                     false, // resample input cloud z
                                                     m_cloud.ref, 2, m_cloud.ref->getOwnBB(),
                                                     true, // fill Empty Cells
                                                     groundHeight,
                                                     false,
                                                     &pDlg, // 新增：进度条
                                                     0 // 新增：保留参数
                                                     ); // export to original cs
*/
        auto rasterCloud = ceilRaster.convertToCloud(
            false, false, {ccRasterGrid::ExportableFields::PER_CELL_AVG_VALUE},
            false, false, false, false, m_cloud.ref, 2, m_cloud.ref->getOwnBB(),
            0.0, true, false, pDlg.get() // 正确：传入pDlg.get()
            );

        if (rasterCloud) {
            int activeSFIndex = rasterCloud->getScalarFieldIndexByName("Height grid values");
            rasterCloud->showSF(activeSFIndex >= 0);
            if (activeSFIndex < 0 && rasterCloud->getNumberOfScalarFields() != 0)
            {
                //if no SF is displayed, we should at least set a valid one (for later)
                activeSFIndex = static_cast<int>(rasterCloud->getNumberOfScalarFields()) - 1;
            }
            rasterCloud->setCurrentDisplayedScalarField(activeSFIndex);

            std::string errorStr;
            /*auto baseMesh = CCCoreLib::PointProjectionTools::computeTriangulation(rasterCloud,
                                                                                  CCCoreLib::TRIANGULATION_TYPES::DELAUNAY_2D_AXIS_ALIGNED, // 三角化类型
                                                                                  static_cast<PointCoordinateType>(CCCoreLib::PointProjectionTools::IGNORE_MAX_EDGE_LENGTH), // 最大边长度（忽略）
                                                                                  2, // 投影轴（Z轴）
                                                                                  errorStr);
            */

            CCCoreLib::GenericIndexedCloudPersist* cloudWrapper = rasterCloud; // 转换为基类指针
            auto baseMesh = CCCoreLib::PointProjectionTools::computeTriangulation(
                cloudWrapper, // 点云指针（正确类型）
                CCCoreLib::TRIANGULATION_TYPES::DELAUNAY_2D_AXIS_ALIGNED, // 三角化类型
                static_cast<PointCoordinateType>(CCCoreLib::PointProjectionTools::IGNORE_MAX_EDGE_LENGTH), // 最大边长度
                2, // 投影轴（Z轴）
                errorStr // 错误信息
                );
            if (baseMesh)
            {
                //m_rasterMesh = new ccMesh(rasterCloud, baseMesh);
                //m_rasterMesh = new ccMesh(rasterCloud); // 仅传入点云
                //m_rasterMesh->setMesh(baseMesh); // 关联网格索引
                m_rasterMesh = new ccMesh(baseMesh, rasterCloud);// 直接通过构造函数关联网格和顶点云
                delete baseMesh;
            }
            if (m_rasterMesh)
            {
                rasterCloud->setEnabled(false);
                rasterCloud->setVisible(true);
                rasterCloud->setName("vertices");

                //   m_rasterMesh->addChild(rasterCloud);
                m_rasterMesh->addChild(rasterCloud);
                m_rasterMesh->setDisplay_recursive(m_glWindow);
                m_rasterMesh->setName(m_cloud.ref->getName() + ".mesh");
                m_rasterMesh->showSF(true);
                m_rasterMesh->showColors(true);

                rotateMatrix.invert();
                m_cloud.ref->rotateGL(rotateMatrix);
                m_cloud.ref->applyGLTransformation_recursive();
                m_cloud.ref->setEnabled(false);
                m_glWindow->addToOwnDB(m_rasterMesh);
                m_glWindow->zoomGlobal();
                m_glWindow->redraw();
            }
        }
        else {
            dispToConsole(QString::fromUtf8("运行内存不足，生成失败"));
        }
    }

    genReportPushButton->setEnabled(true);
    switchPcMeshPushButton->setEnabled(true);
    nProgress.oneStep();
    return;
}

void qVolumeMeasureDlg::onGenReportPushButtonClick()
{
    // check density
    {
        auto density = densityLineEdit->text().toFloat();
        if (density < 0 || density > 23) {
            QMessageBox::warning(this,
                                 QString::fromUtf8("警告"),
                                 QString::fromUtf8("请输入合理的密度值 0.0 ~ 23.0"));
            return;
        }
    }

    // get file name
    QString fileName;
    QString imgName;
    QTemporaryFile img("XXXXXX.jpg", this);
    {
        QString defaultName = QString::fromUtf8("测量报告")
                              + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")
                              + ".doc";

        fileName = QFileDialog::getSaveFileName(
            this,
            QString::fromUtf8("保存文件"),
            currentPath.absoluteFilePath(defaultName),
            "Doc (*.doc)"
            );

        if (fileName.isEmpty()) return;

        img.open();
        imgName = img.fileName();
    }

    // gen progress dialog
    auto pDlg = std::make_unique<ccProgressDialog>(false, this);
    {
        pDlg->setMethodTitle(QString::fromUtf8("生成报告......"));
        pDlg->start();
        pDlg->show();
        QCoreApplication::processEvents();
    }
    CCCoreLib::NormalizedProgress nProgress(pDlg.get(), 9);

    // create screen short
    {
        pDlg->setInfo(QString::fromUtf8("生成点云截图 ..."));
        m_glWindow->renderToFile(img.fileName());
        nProgress.oneStep();
    }

    // create doc file
    {
        pDlg->setInfo(QString::fromUtf8("生成 Word 文档 ..."));
        QScopedPointer<WordAppType> wordApp(new WordAppType("Word.Application", this, Qt::MSWindowsOwnDC));
        /*if (!m_word.get())
            m_word = std::make_unique<QAxWidget>("Word.Application", this, Qt::MSWindowsOwnDC);*/
        wordApp->setProperty("Visible", false);
        auto documents = wordApp->querySubObject("Documents");
        documents->dynamicCall("Add(QString)", currentPath.absoluteFilePath("template.dot"));
        auto document = wordApp->querySubObject("ActiveDocument");
        nProgress.oneStep();

        auto bookmarkTime = document->querySubObject("Bookmarks(QVariant)", "GenerateTime");
        if (!bookmarkTime->isNull()) {
            bookmarkTime->dynamicCall("Select(void)");
            bookmarkTime->querySubObject("Range")
                ->setProperty("Text", QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
            nProgress.oneStep();
        }

        auto bookmarkVolume = document->querySubObject("Bookmarks(Volume)");
        if (!bookmarkVolume->isNull()) {
            bookmarkVolume->dynamicCall("Select(void)");
            bookmarkVolume->querySubObject("Range")
                ->setProperty("Text", QString::number(report.volumeAbove, 'f', 3));
            nProgress.oneStep();
        }

        auto bookmarkDensity = document->querySubObject("Bookmarks(QVariant)", "Density");
        if (!bookmarkDensity->isNull()) {
            bookmarkDensity->dynamicCall("Select(void)");
            bookmarkDensity->querySubObject("Range")
                ->setProperty("Text",QString::number(report.density, 'f', 3));
            nProgress.oneStep();
        }

        auto bookmarkWeight = document->querySubObject("Bookmarks(QVariant)", "Weight");
        if (!bookmarkWeight->isNull()) {
            bookmarkWeight->dynamicCall("Select(void)");
            bookmarkWeight->querySubObject("Range")
                ->setProperty("Text", QString::number(report.volumeAbove* report.density, 'f', 3));
            nProgress.oneStep();
        }

        auto bookmarkPicture = document->querySubObject("Bookmarks(QVariant)", "Picture");
        if (!bookmarkPicture->isNull()) {
            bookmarkPicture->dynamicCall("Select(void)");
            auto tmp = document->querySubObject("InlineShapes");
            tmp->dynamicCall("AddPicture(const QString&)", imgName);
            nProgress.oneStep();
        }

        auto bookmarkDetail = document->querySubObject("Bookmarks(QVariant)", "Detail");
        if (!bookmarkDetail->isNull()) {
            bookmarkDetail->dynamicCall("Select(void)");
            bookmarkDetail->querySubObject("Range")
                ->setProperty("Text", QString::fromUtf8(
                                          "地上体积：%1 立方米\n"
                                          "地下体积：%2 立方米\n"
                                          ).arg(report.volumeAbove, 0, 'f', 3).arg(report.volumeUnder, 0, 'f', 3));
            nProgress.oneStep();
        }

        pDlg->setInfo(QString::fromUtf8("保存报告 ..."));
        document->dynamicCall("SaveAs(const QString&)", fileName);
        document->dynamicCall("Close(boolean)", true);
        dispToConsole(QString::fromUtf8("报告已保存到：%1").arg(fileName));
    }

    nProgress.oneStep();
    return;
}

void qVolumeMeasureDlg::onOkPushButtonClick()
{
    clearPushButton->setEnabled(false);
    calPushButton->setEnabled(true);
    okPushButton->setEnabled(false);

    m_label.setVisible(false);

    m_cloud.ref->showSF(false);
    m_cloud.ref->setColor(ccColor::white);

    m_glWindow->redraw();

    dispToConsole(QString::fromUtf8("基准地面保存成功！"));
}

void qVolumeMeasureDlg::onSwitchPushButtonClick()
{
    clearPushButton->setEnabled(true);

    if (m_cloud.ref->isEnabled())
    {
        m_cloud.ref->setEnabled(false);
        m_rasterMesh->setEnabled(true);
    }
    else
    {
        m_cloud.ref->setEnabled(true);
        m_rasterMesh->setEnabled(false);
    }
    m_glWindow->redraw();
}

/*void qVolumeMeasureDlg::handlePickedItem(ccHObject* entity, unsigned itemIdx, int x, int y, const CCVector3& p, const CCVector3d& uwv)
{
    if (m_pointsIdx.size() < 3 && entity)
    {
        m_label.addPickedPoint(static_cast<ccGenericPointCloud*>(entity), itemIdx);
        m_pointsIdx.push_back(itemIdx);
        m_glWindow->redraw();
        clearPushButton->setEnabled(m_pointsIdx.size() > 0);
        okPushButton->setEnabled(m_pointsIdx.size() == 3);

        dispToConsole(QString::fromLocal8Bit("点 %4 (%1, %2, %3)")
                          .arg(p.x).arg(p.y).arg(p.z)
                          .arg(m_pointsIdx.size()));
    }
    else
    {
        dispToConsole(QString::fromLocal8Bit("无效点，已舍弃"));
    }
}*/

/*void qVolumeMeasureDlg::handlePickedItem(ccHObject* entity, int itemIdx, int x, int y)
{
    // 过滤非点云实体
    if (!entity || !entity->isA(CC_TYPES::POINT_CLOUD)) {
        dispToConsole(QString::fromLocal8Bit("请选择点云实体！"));
        return;
    }

    // 限制选点数量为3个
    if (m_pointsIdx.size() < 3) {
        ccGenericPointCloud* cloud = static_cast<ccGenericPointCloud*>(entity);
        // 验证点索引有效性
        if (itemIdx < 0 || itemIdx >= cloud->size()) {
            dispToConsole(QString::fromLocal8Bit("无效点索引！"));
            return;
        }

        // 添加点到标记和索引列表
        m_label.addPickedPoint(cloud, itemIdx);
        m_pointsIdx.push_back(static_cast<unsigned>(itemIdx)); // 转换为unsigned
        m_glWindow->redraw();

        // 更新按钮状态
        clearPushButton->setEnabled(m_pointsIdx.size() > 0);
        okPushButton->setEnabled(m_pointsIdx.size() == 3);

        // 显示点坐标
        CCVector3 p = cloud->getPoint(static_cast<unsigned>(itemIdx));
        dispToConsole(QString::fromLocal8Bit("已选点 %1：(%2, %3, %4)")
                          .arg(m_pointsIdx.size())
                          .arg(p.x).arg(p.y).arg(p.z));
    } else {
        dispToConsole(QString::fromLocal8Bit("已选满3个点，请点击确认！"));
    }
}
*/

/*void qVolumeMeasureDlg::handlePickedItem(ccHObject* entity, unsigned itemIdx, const CCVector3& P)
{
    // 检查是否拾取到点云实体
    if (m_pointsIdx.size() < 3 && entity && entity->isA(CC_TYPES::POINT_CLOUD)) {
        // 将实体转换为点云
        ccGenericPointCloud* cloud = static_cast<ccGenericPointCloud*>(entity);
        // 记录点索引
        m_pointsIdx.push_back(itemIdx);
        // 在3D窗口中标记选点
        m_label.addPickedPoint(cloud, itemIdx);
        // 刷新窗口显示
        m_glWindow->redraw();
        // 更新按钮状态
        clearPushButton->setEnabled(true);  // 选点后启用“清空”按钮
        okPushButton->setEnabled(m_pointsIdx.size() == 3);  // 选满3个点启用“确认”按钮
        // 在控制台显示点坐标
        dispToConsole(QString::fromLocal8Bit("已选点 %1：(%2, %3, %4)")
                          .arg(m_pointsIdx.size())
                          .arg(P.x).arg(P.y).arg(P.z));
    } else if (m_pointsIdx.size() >= 3) {
        dispToConsole(QString::fromLocal8Bit("已选满3个点，请点击“确认”按钮！"));
    } else {
        dispToConsole(QString::fromLocal8Bit("请点击点云选取有效点！"));
    }
}
*/

void qVolumeMeasureDlg::handlePickedItem(ccHObject* entity, unsigned itemIdx, int x, int y, const CCVector3& P, const CCVector3d& uwv)
{
    // 原函数逻辑不变，忽略新增的x, y, uwv参数
    if (m_pointsIdx.size() < 3 && entity && entity->isA(CC_TYPES::POINT_CLOUD)) {
        ccGenericPointCloud* cloud = static_cast<ccGenericPointCloud*>(entity);
        m_pointsIdx.push_back(itemIdx);
        m_label.addPickedPoint(cloud, itemIdx);
        m_glWindow->redraw();
        clearPushButton->setEnabled(true);
        okPushButton->setEnabled(m_pointsIdx.size() == 3);
        dispToConsole(QString::fromUtf8("已选点 %1：(%2, %3, %4)")
                          .arg(m_pointsIdx.size())
                          .arg(P.x).arg(P.y).arg(P.z));
    } else if (m_pointsIdx.size() >= 3) {
        dispToConsole(QString::fromUtf8("已选满3个点，请点击“确认”按钮！"));
    } else {
        dispToConsole(QString::fromUtf8("请点击点云选取有效点！"));
    }
}



void qVolumeMeasureDlg::dispToConsole(const QString& msg)
{
    textBrowser->append(msg);
}


void qVolumeMeasureDlg::CloudBackup::backup(ccPointCloud* cloud)
{
    //save state
    assert(!colors);
    wasVisible = cloud->isVisible();
    wasEnabled = cloud->isEnabled();
    wasSelected = cloud->isSelected();
    hadColors = cloud->hasColors();
    displayedSFIndex = cloud->getCurrentDisplayedScalarFieldIndex();
    originDisplay = cloud->getDisplay();
    colorsWereDisplayed = cloud->colorsShown();
    sfWasDisplayed = cloud->sfShown();
    hadOctree = (cloud->getOctree() != nullptr);
    ref = cloud;
}

bool qVolumeMeasureDlg::CloudBackup::backupColors()
{
    if (!ref)
    {
        assert(false);
        return false;
    }

    //we backup the colors (as we are going to change them)
    if (ref->hasColors())
    {
        colors = new RGBAColorsTableType;
        if (!colors->resizeSafe(ref->size()))
        {
            //not enough memory
            colors->release();
            colors = nullptr;
            return false;
        }

        //copy the existing colors
        for (unsigned i = 0; i < ref->size(); ++i)
        {
            colors->setValue(i, ref->getPointColor(i));
        }
    }

    return true;
}

void qVolumeMeasureDlg::CloudBackup::restore()
{
    if (!ref)
    {
        //nothing to do
        return;
    }

    if (!hadOctree)
    {
        //we can only delete the octree if it has not already been added to the DB tree!!!
        if (!ref->getParent())
        {
            ref->deleteOctree();
        }
    }

    if (hadColors)
    {
        //restore original colors
        if (colors)
        {
            assert(ref->hasColors());
            for (unsigned i = 0; i < ref->size(); ++i)
            {
                ref->setPointColor(i, colors->getValue(i));
            }
        }
    }
    else
    {
        ref->unallocateColors();
    }

    ref->setEnabled(wasEnabled);
    ref->setVisible(wasVisible);
    ref->setSelected(wasSelected);
    ref->showColors(colorsWereDisplayed);
    ref->showSF(sfWasDisplayed);
    ref->setCurrentDisplayedScalarField(displayedSFIndex);
    ref->setDisplay(originDisplay);
}

void qVolumeMeasureDlg::CloudBackup::clear()
{
    if (colors)
    {
        colors->release();
        colors = nullptr;
    }

    if (ref)
    {
        if (ownCloud)
        {
            //the dialog takes care of its own clouds!
            delete ref;
        }
        ref = nullptr;
    }
}
