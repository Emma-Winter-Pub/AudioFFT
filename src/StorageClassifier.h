#pragma once

#include "StorageTypes.h"

class StorageClassifier {
public:
    static void synthesizeTopology(VolumeTopology& topology);

private:
    static void classifySingleDevice(PhysicalDevice& device);
};