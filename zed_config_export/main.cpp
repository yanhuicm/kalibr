#include <sl/Camera.hpp>

std::vector<float> split(std::string str, std::string token);
std::string keepUseful(std::string& str);
void setValues(sl::CameraParameters &param, std::vector<float> &values);
void printCalib(sl::CameraParameters& param);

static const std::vector<std::pair<int, int>> cameraResolution = {
    std::make_pair(2208, 1242), /**< sl::RESOLUTION_HD2K */
    std::make_pair(1920, 1080), /**< sl::RESOLUTION_HD1080 */
    std::make_pair(1280, 720), /**< sl::RESOLUTION_HD720 */
    std::make_pair(672, 376) /**< sl::RESOLUTION_VGA */
};

int wBase = 2688;
int hBase = 1520;
double offsetQVGA = 5.;
double offsetHD = 3.;
double cropBaseQVGA_w = (wBase - cameraResolution[3].first * 4.) / 2. + offsetQVGA; // *4 because of binning 4x4
double cropBaseQVGA_h = (hBase - cameraResolution[3].second * 4.) / 2. + offsetQVGA; // *4 because of binning 4x4
double cropBaseHD_w = (wBase - cameraResolution[2].first * 2.) / 2. + offsetHD; // *2 because of binning 2x2
double cropBaseHD_h = (hBase - cameraResolution[2].second * 2.) / 2. + offsetHD; // *2 because of binning 2x2
double cropBase2K_w = (wBase - cameraResolution[0].first) / 2.;
double cropBase2K_h = (hBase - cameraResolution[0].second) / 2.;
double crop2K_w = (cameraResolution[0].first - cameraResolution[1].first) / 2;
double crop2K_h = (cameraResolution[0].second - cameraResolution[1].second) / 2;

inline void print(FILE* fich, std::string str, float v) {
    fprintf(fich, "%s=%f\n", str.c_str(), v);
}

void printf(FILE* fich, std::string side_name, sl::CameraParameters &param) {
    fprintf(fich, "[%s]\n", side_name.c_str());
    print(fich, "fx", param.fx);
    print(fich, "fy", param.fy);
    print(fich, "cx", param.cx);
    print(fich, "cy", param.cy);
    print(fich, "k1", param.disto[0]);
    print(fich, "k2", param.disto[1]);
    print(fich, "p1", param.disto[2]);
    print(fich, "p2", param.disto[3]);
    print(fich, "k3", param.disto[4]);
    fprintf(fich, "\n");
}

std::string getRes(int width) {
    std::string str;
    switch (width) {
        case 2208: str = "2K";
            break;
        case 1920:str = "FHD";
            break;
        case 1280:str = "HD";
            break;
        case 672:str = "VGA";
            break;
    }
    return str;
}

void cvtTo2K(sl::CameraParameters& param) {
    param.cx = (param.cx * 2.) + cropBaseHD_w - cropBase2K_w;
    param.cy = (param.cy * 2.) + cropBaseHD_h - cropBase2K_h;
    param.fx *= 2.;
    param.fy *= 2.;
}

sl::CameraParameters getCameraMatrixByRes(sl::CameraParameters& ref, int width) {
    sl::CameraParameters matrix;

    double CxBase = ref.cx + cropBase2K_w;
    double CyBase = ref.cy + cropBase2K_h;

    switch (width) {
        case 2208:
            matrix.cx = ref.cx;
            matrix.cy = ref.cy;
            matrix.fx = ref.fx;
            matrix.fy = ref.fy;
            break;
        case 1920:
            matrix.cx = (CxBase - cropBase2K_w) - crop2K_w;
            matrix.cy = (CyBase - cropBase2K_h) - crop2K_h;
            matrix.fx = ref.fx;
            matrix.fy = ref.fy;
            break;
        case 1280:
            matrix.cx = (CxBase - cropBaseHD_w) / 2.;
            matrix.cy = (CyBase - cropBaseHD_h) / 2.;
            matrix.fx = ref.fx / 2.;
            matrix.fy = ref.fy / 2.;
            break;
        case 672:
            matrix.cx = (CxBase - cropBaseQVGA_w) / 4.;
            matrix.cy = (CyBase - cropBaseQVGA_h) / 4.;
            matrix.fx = ref.fx / 4.;
            matrix.fy = ref.fy / 4.;
            break;
    }

    for (int d = 0; d < 5; d++)
        matrix.disto[d] = ref.disto[d];

    return matrix;
}

void writeFullCalibration(std::string fileName, sl::CalibrationParameters& calib) {
    FILE* fich = fopen(fileName.c_str(), "w");
    if (fich == nullptr) {
        std::cout << "Error write in " << fileName << "\n";
        return;
    }

    printf(fich, "LEFT_CAM_HD", calib.left_cam);
    printf(fich, "RIGHT_CAM_HD", calib.right_cam);

    fprintf(fich, "[%s]\n", "STEREO");

    sl::Transform rt;
#if ((ZED_SDK_MAJOR_VERSION >= 3)&&(ZED_SDK_MINOR_VERSION >= 1))
    rt = calib.stereo_transform;
#else
    rt.setIdentity();
    rt.setRotationVector(calib.R);
    rt.setTranslation(calib.T);
#endif
    auto rot_ = rt.getRotationVector();
    auto trans = rt.getTranslation();

    print(fich, "Baseline", trans.x * -1000.f);

    print(fich, "TY", trans.y);
    print(fich, "TZ", trans.z);

    // Read translations
    print(fich, "RX_HD", rot_.x);
    print(fich, "CV_HD", rot_.y);
    print(fich, "RZ_HD", rot_.z);
    fprintf(fich, "\n");
    fclose(fich);
}

int main(int argc, char** argv) {
    std::string filename(argv[1]);
    std::string serial_str(argv[2]);
    std::ifstream kalibr_file(filename);

    sl::CalibrationParameters zed_calib;
    memset(zed_calib.left_cam.disto, 0, 5 * sizeof (double));
    memset(zed_calib.right_cam.disto, 0, 5 * sizeof (double));

#if ((ZED_SDK_MAJOR_VERSION >= 3)&&(ZED_SDK_MINOR_VERSION >= 1))
    zed_calib.stereo_transform.setIdentity();
#endif
    bool side_left = true;
    std::string line;
    // convert our file line by line
    while (std::getline(kalibr_file, line)) {
        if (line.find("distortion:") != std::string::npos) {
            auto values = keepUseful(line);
            std::vector<float> disto = split(values, " ");
            for (int d = 0; d < disto.size(); d++)
                if (side_left) zed_calib.left_cam.disto[d] = disto[d];
                else zed_calib.right_cam.disto[d] = disto[d];
        }

        if (line.find("projection:") != std::string::npos) {
            auto values = keepUseful(line);
            std::vector<float> K = split(values, " ");
            if (side_left) {
                setValues(zed_calib.left_cam, K);
                side_left = false;
            } else
                setValues(zed_calib.right_cam, K);
        }

        if (line.find("q:") != std::string::npos) {
            auto values = keepUseful(line);
            std::vector<float> q = split(values, " ");
            sl::Orientation orientation;
            orientation.ox = q[0];
            orientation.oy = q[1];
            orientation.oz = q[2];
            orientation.ow = q[3];
#if ((ZED_SDK_MAJOR_VERSION >= 3)&&(ZED_SDK_MINOR_VERSION >= 1))
            zed_calib.stereo_transform.setOrientation(orientation);
#else
            zed_calib.R = orientation.getRotation().getRotationVector();
#endif
        }

        if (line.find("t:") != std::string::npos) {
            auto values = keepUseful(line);
            std::vector<float> t = split(values, " ");
            sl::float3 translation;
            translation.x = t[0];
            translation.y = t[1];
            translation.z = t[2];
#if ((ZED_SDK_MAJOR_VERSION >= 3)&&(ZED_SDK_MINOR_VERSION >= 1))
            zed_calib.stereo_transform.setTranslation(translation);
#else
            zed_calib.T = translation;
#endif
        }
    }
    kalibr_file.close();

#if 0
    printCalib(zed_calib.left_cam);
    printCalib(zed_calib.right_cam);
    zed_calib.stereo_transform.matrix_name = "stereo_transform";
    std::cout << zed_calib.stereo_transform.getInfos() << "\n";
#endif

    writeFullCalibration("SN" + serial_str + ".conf", zed_calib);

    return EXIT_SUCCESS;
}

std::vector<float> split(std::string str, std::string token) {
    // split the formated line into usefull string
    std::vector<std::string> v_str;
    while (str.size()) {
        int index = str.find(token);
        if (index != std::string::npos) {
            v_str.push_back(str.substr(0, index));
            str = str.substr(index + token.size());
            if (str.size() == 0)v_str.push_back(str);
        } else {
            v_str.push_back(str);
            str = "";
        }
    }
    std::vector<float> v_float;
    for (auto it : v_str) {
        if (!it.empty())
            v_float.push_back(std::atof(it.c_str()));
    }
    return v_float;
}

std::string keepUseful(std::string& str) {
    std::string str_out = str;
    auto f_ = str_out.find_first_of("[");
    str_out = str_out.substr(f_ + 1);
    f_ = str_out.find_first_of("]");
    str_out = str_out.substr(0, f_);
    return str_out;
}

void setValues(sl::CameraParameters& param, std::vector<float>& values) {
    param.fx = values[0];
    param.fy = values[1];
    param.cx = values[2];
    param.cy = values[3];
}

void printCalib(sl::CameraParameters& param) {
    std::cout << "fx: " << param.fx << " fy: " << param.fy << "\ncx: " << param.cx << " cy: " << param.cy << "\n";
    std::cout << "k1: " << param.disto[0] << " k2: " << param.disto[1] << " p1: " << param.disto[2] << " p2: " << param.disto[3] << " k3: " << param.disto[4] << "\n";
}