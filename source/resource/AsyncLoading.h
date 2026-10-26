#pragma once
#include "ResourceConfig.h"
#include <cstdint>
#include <string>
#include <vector>

using AsyncLoadId=std::uint64_t;
enum class AssetKind {Mesh,Image,Material,Model,Shader,Text,Json,Scene};
struct AssetRequest {
    AssetKind kind;
    std::string file;
    EColorFormat format=EColorFormat::RGB24;
};
enum class LoadPhase {Preparing,AwaitingUpload,Uploading,Ready,Failed,Cancelled};
struct AsyncLoadStatus {
    LoadPhase phase=LoadPhase::Preparing;
    unsigned prepared=0,total=0,published=0,uploadTotal=0;
    std::string error;
    bool terminal() const {return phase==LoadPhase::Ready || phase==LoadPhase::Failed || phase==LoadPhase::Cancelled;}
};
