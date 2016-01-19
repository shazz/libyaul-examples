const int lut[512] = {
        2, 3, 5, 6, 8, 9, 11, 13,
        14, 16, 17, 19, 20, 22, 23, 25,
        27, 28, 30, 31, 33, 34, 36, 37,
        39, 40, 42, 43, 45, 46, 48, 49,
        50, 52, 53, 55, 56, 58, 59, 60,
        62, 63, 64, 66, 67, 68, 70, 71,
        72, 74, 75, 76, 78, 79, 80, 81,
        82, 84, 85, 86, 87, 88, 89, 91,
        92, 93, 94, 95, 96, 97, 98, 99,
        100, 101, 102, 103, 104, 105, 106, 106,
        107, 108, 109, 110, 111, 111, 112, 113,
        114, 114, 115, 116, 116, 117, 118, 118,
        119, 119, 120, 121, 121, 122, 122, 122,
        123, 123, 124, 124, 125, 125, 125, 126,
        126, 126, 126, 127, 127, 127, 127, 127,
        128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 127,
        127, 127, 127, 127, 126, 126, 126, 126,
        125, 125, 125, 124, 124, 123, 123, 122,
        122, 122, 121, 121, 120, 119, 119, 118,
        118, 117, 116, 116, 115, 114, 114, 113,
        112, 111, 111, 110, 109, 108, 107, 106,
        106, 105, 104, 103, 102, 101, 100, 99,
        98, 97, 96, 95, 94, 93, 92, 91,
        89, 88, 87, 86, 85, 84, 82, 81,
        80, 79, 78, 76, 75, 74, 72, 71,
        70, 68, 67, 66, 64, 63, 62, 60,
        59, 58, 56, 55, 53, 52, 50, 49,
        48, 46, 45, 43, 42, 40, 39, 37,
        36, 34, 33, 31, 30, 28, 27, 25,
        23, 22, 20, 19, 17, 16, 14, 13,
        11, 9, 8, 6, 5, 3, 2, 0,
        -2, -3, -5, -6, -8, -9, -11, -13,
        -14, -16, -17, -19, -20, -22, -23, -25,
        -27, -28, -30, -31, -33, -34, -36, -37,
        -39, -40, -42, -43, -45, -46, -48, -49,
        -50, -52, -53, -55, -56, -58, -59, -60,
        -62, -63, -64, -66, -67, -68, -70, -71,
        -72, -74, -75, -76, -78, -79, -80, -81,
        -82, -84, -85, -86, -87, -88, -89, -91,
        -92, -93, -94, -95, -96, -97, -98, -99,
        -100, -101, -102, -103, -104, -105, -106, -106,
        -107, -108, -109, -110, -111, -111, -112, -113,
        -114, -114, -115, -116, -116, -117, -118, -118,
        -119, -119, -120, -121, -121, -122, -122, -122,
        -123, -123, -124, -124, -125, -125, -125, -126,
        -126, -126, -126, -127, -127, -127, -127, -127,
        -128, -128, -128, -128, -128, -128, -128, -128,
        -128, -128, -128, -128, -128, -128, -128, -127,
        -127, -127, -127, -127, -126, -126, -126, -126,
        -125, -125, -125, -124, -124, -123, -123, -122,
        -122, -122, -121, -121, -120, -119, -119, -118,
        -118, -117, -116, -116, -115, -114, -114, -113,
        -112, -111, -111, -110, -109, -108, -107, -106,
        -106, -105, -104, -103, -102, -101, -100, -99,
        -98, -97, -96, -95, -94, -93, -92, -91,
        -89, -88, -87, -86, -85, -84, -82, -81,
        -80, -79, -78, -76, -75, -74, -72, -71,
        -70, -68, -67, -66, -64, -63, -62, -60,
        -59, -58, -56, -55, -53, -52, -50, -49,
        -48, -46, -45, -43, -42, -40, -39, -37,
        -36, -34, -33, -31, -30, -28, -27, -25,
        -23, -22, -20, -19, -17, -16, -14, -13,
        -11, -9, -8, -6, -5, -3, -2, 0
};