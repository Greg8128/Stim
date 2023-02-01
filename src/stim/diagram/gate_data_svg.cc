#include "stim/diagram/gate_data_svg.h"

using namespace stim_draw_internal;

std::map<std::string, SvgGateData> SvgGateData::make_gate_data_map() {
    std::map<std::string, SvgGateData> result;

    result.insert({"X", {1, "X", "", "", "white", "black", 0, 10}});
    result.insert({"Y", {1, "Y", "", "", "white", "black", 0, 10}});
    result.insert({"Z", {1, "Z", "", "", "white", "black", 0, 10}});

    result.insert({"H_YZ", {1, "H", "YZ", "", "white", "black", 22, 12}});
    result.insert({"H", {1, "H", "", "", "white", "black", 0, 10}});
    result.insert({"H_XY", {1, "H", "XY", "", "white", "black", 22, 12}});

    result.insert({"SQRT_X", {1, "√X", "", "", "white", "black", 24}});
    result.insert({"SQRT_Y", {1, "√Y", "", "", "white", "black", 24}});
    result.insert({"S", {1, "S", "", "", "white", "black", 0, 10}});

    result.insert({"SQRT_X_DAG", {1, "√X", "", "†", "white", "black", 18, 14}});
    result.insert({"SQRT_Y_DAG", {1, "√Y", "", "†", "white", "black", 18, 14}});
    result.insert({"S_DAG", {1, "S", "", "†", "white", "black", 26, 14}});

    result.insert({"MX", {1, "M", "X", "", "black", "white", 26, 16}});
    result.insert({"MY", {1, "M", "Y", "", "black", "white", 26, 16}});
    result.insert({"M", {1, "M", "", "", "black", "white", 0, 10}});

    result.insert({"RX", {1, "R", "X", "", "black", "white", 26, 16}});
    result.insert({"RY", {1, "R", "Y", "", "black", "white", 26, 16}});
    result.insert({"R", {1, "R", "", "", "black", "white", 0, 10}});

    result.insert({"MRX", {1, "MR", "X", "", "black", "white", 0, 14}});
    result.insert({"MRY", {1, "MR", "Y", "", "black", "white", 0, 14}});
    result.insert({"MR", {1, "MR", "", "", "black", "white", 24, 16}});

    result.insert({"X_ERROR", {1, "ERR", "X", "", "pink", "black", 0, 10}});
    result.insert({"Y_ERROR", {1, "ERR", "Y", "", "pink", "black", 0, 10}});
    result.insert({"Z_ERROR", {1, "ERR", "Z", "", "pink", "black", 0, 10}});

    result.insert({"E[X]", {1, "E", "X", "", "pink", "black", 0, 10}});
    result.insert({"E[Y]", {1, "E", "Y", "", "pink", "black", 0, 10}});
    result.insert({"E[Z]", {1, "E", "Z", "", "pink", "black", 0, 10}});

    result.insert({"ELSE_CORRELATED_ERROR[X]", {1, "EE", "X", "", "pink", "black", 0, 10}});
    result.insert({"ELSE_CORRELATED_ERROR[Y]", {1, "EE", "Y", "", "pink", "black", 0, 10}});
    result.insert({"ELSE_CORRELATED_ERROR[Z]", {1, "EE", "Z", "", "pink", "black", 0, 10}});

    result.insert({"MPP[X]", {1, "MPP", "X", "", "black", "white", 0, 10}});
    result.insert({"MPP[Y]", {1, "MPP", "Y", "", "black", "white", 0, 10}});
    result.insert({"MPP[Z]", {1, "MPP", "Z", "", "black", "white", 0, 10}});

    result.insert({"SQRT_XX", {1, "√XX", "", "", "white", "black", 0, 10}});
    result.insert({"SQRT_YY", {1, "√YY", "", "", "white", "black", 0, 10}});
    result.insert({"SQRT_ZZ", {1, "√ZZ", "", "", "white", "black", 0, 10}});

    result.insert({"SQRT_XX_DAG", {1, "√XX", "", "†", "white", "black", 0, 10}});
    result.insert({"SQRT_YY_DAG", {1, "√YY", "", "†", "white", "black", 0, 10}});
    result.insert({"SQRT_ZZ_DAG", {1, "√ZZ", "", "†", "white", "black", 0, 10}});

    result.insert({"I", {1, "I", "", "", "white", "black", 0, 10}});
    result.insert({"C_XYZ", {1, "C", "XYZ", "", "white", "black", 18, 10}});
    result.insert({"C_ZYX", {1, "C", "ZYX", "", "white", "black", 18, 10}});

    result.insert({"DEPOLARIZE1", {1, "DEP", "1", "", "pink", "black", 0, 10}});
    result.insert({"DEPOLARIZE2", {1, "DEP", "2", "", "pink", "black", 0, 10}});

    result.insert({"PAULI_CHANNEL_1", {4, "PAULI_CHANNEL_1", "", "", "pink", "black", 0, 10}});
    result.insert({"PAULI_CHANNEL_2[0]", {16, "PAULI_CHANNEL_2", "0", "", "pink", "black", 0, 10}});
    result.insert({"PAULI_CHANNEL_2[1]", {16, "PAULI_CHANNEL_2", "1", "", "pink", "black", 0, 10}});

    return result;
}
