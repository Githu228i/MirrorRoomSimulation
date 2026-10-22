#ifndef CONSTANTS_H
#define CONSTANTS_H

// ============================================================
//  Central place for all named constants of MirrorRoomSimulation.
//  To change a "magic number" of the app, edit it here.
//  Namespaces and names match the previous per-file declarations,
//  so the rest of the code keeps compiling unchanged.
// ============================================================

#include <QColor>
#include <limits>

// ------------------------------------------------------------
//  Dark theme: colors and widget metrics
// ------------------------------------------------------------
namespace Theme {

// Window / working area
inline const QColor Window       (0x1b, 0x1d, 0x21);
inline const QColor Canvas       (0x12, 0x14, 0x17);
inline const QColor CanvasBorder (0x34, 0x38, 0x40);

// Controls
inline const QColor Panel      (0x26, 0x29, 0x2e);
inline const QColor PanelHover (0x30, 0x34, 0x3b);
inline const QColor PanelPress (0x1f, 0x22, 0x26);
inline const QColor Border     (0x3a, 0x3e, 0x46);
inline const QColor Text       (0xe6, 0xe8, 0xeb);
inline const QColor TextMuted  (0x8b, 0x91, 0x9b);
inline const QColor Accent     (0x4d, 0xa3, 0xff);

// Room rendering
inline const QColor Wall     (0xe6, 0xe8, 0xeb);
inline const QColor Source   (0xff, 0x52, 0x52);
inline const QColor Trace    (0xff, 0xd5, 0x4a);
inline const QColor Reflect  (0xff, 0x9f, 0x1a);

// Widget sizes (px)
inline constexpr int ButtonRadius   = 10;
inline constexpr int CheckSize      = 26;
inline constexpr int SliderGroove   = 6;
inline constexpr int SliderHandle   = 20;
inline constexpr int ScrollBarWidth = 10;

} // namespace Theme

// ------------------------------------------------------------
//  Working-area limits and input validation thresholds
// ------------------------------------------------------------
namespace Limits {
constexpr double MinX = 0.0;
constexpr double MaxX = 1580.0;
constexpr double MinY = 0.0;
constexpr double MaxY = 1000.0;
constexpr double MinPointDist      = 10.0;
constexpr int    MinWalls          = 4;
constexpr int    MaxWalls          = 9;
constexpr double ClickRadius       = 5.0;
constexpr double BeamWallTolerance = 6.0;
constexpr double MinBeamVertexDist = 10.0;
}

// ------------------------------------------------------------
//  Drawing sizes, UI defaults and animation
// ------------------------------------------------------------
namespace Style {
constexpr int    WallWidth           = 5;
constexpr double PointRadius         = 4.0;
constexpr double BeamRadius          = 4.0;
constexpr int    TraceWidth          = 2;
constexpr int    ReflectPointWidth   = WallWidth + 1;
constexpr double RaySpeed            = 500.0;
constexpr int    RaySpeedMin         = 100;
constexpr int    RaySpeedMax         = 2000;
constexpr int    RaySpeedStep        = 50;
constexpr int    RepeatDelayMin      = 0;
constexpr int    RepeatDelayMax      = 5000;
constexpr int    RepeatDelayStep     = 100;
constexpr int    RepeatDelayDefault  = 500;
constexpr int    MaxChatMessages     = 500;
constexpr int    TraceLimitDefault   = 50;
constexpr int    TraceLimitMin       = 1;
constexpr int    TraceLimitMax       = 1000000;
constexpr bool   EraseOldDefault     = false;

// Coordinates are stored as double (matches Points::getX()).
using CoordType = double;

constexpr int    SaveDecimals      = 3;
constexpr int    SaveDecimalsMax   = std::numeric_limits<CoordType>::max_digits10;
constexpr int    SaveAngleDecimals = 6;
constexpr int    WindowWidth       = 2000;
constexpr int    WindowHeight      = 1300;
constexpr double DimOpacity        = 0.35;
constexpr int    MessageDecimals   = 3;
constexpr int    DelayDecimals     = 1;
constexpr double MsPerSecond       = 1000.0;
constexpr int    MinAnimMs         = 1;
}

// ------------------------------------------------------------
//  Math
// ------------------------------------------------------------
namespace Math {
constexpr double Pi          = 3.14159265358979323846;
constexpr double TwoPi       = 2.0 * Pi;
constexpr double HalfTurnDeg = 180.0;
constexpr double FullTurnDeg = 360.0;
}

// ------------------------------------------------------------
//  Mirror kinds (must match ChangePointData::mirrorType
//  and TraceSnapshot::Wall::type)
// ------------------------------------------------------------
namespace MirrorKind {
constexpr int Plane   = 0;
constexpr int Concave = 1;
constexpr int Convex  = 2;
}

// ------------------------------------------------------------
//  Allowed beam angle (degrees)
// ------------------------------------------------------------
namespace BeamAngle {
constexpr double Min      = 0.01;
constexpr double Max      = 179.99;
constexpr int    Decimals = 2;
}

// ------------------------------------------------------------
//  Arc drawing (mirror.h)
// ------------------------------------------------------------
namespace MirrorDraw {
constexpr double SegmentLength = 3.0;
constexpr int    MinSegments   = 8;
constexpr int    MaxSegments   = 2000;
}

// ------------------------------------------------------------
//  Dialog sizes (px)
// ------------------------------------------------------------
namespace DialogSizes {
constexpr int ChangeOrConnectWidth  = 480;
constexpr int ChangeOrConnectHeight = 200;
// The other dialogs use inline setFixedSize(450, 280/330); add entries
// here if you want them centralized as well.
}

// ------------------------------------------------------------
//  Geometric epsilons (mirror.cpp / polygon.cpp)
// ------------------------------------------------------------
namespace GeoEps {
constexpr double EPS      = 1e-9;
constexpr double POINT_EPS = 1e-6;
constexpr double PAR_EPS  = 1e-10;
constexpr double FLAT_R    = 1e7;
constexpr double TRACE_EPS = 1e-7;
}

// ------------------------------------------------------------
//  FrontEndOperators internal epsilons / scan resolution
// ------------------------------------------------------------
namespace FeEps {
constexpr double SamePointEps  = 1e-6;
constexpr double NoMoveEps     = 1e-9;
constexpr int    WallScanSteps = 120;
}

#endif // CONSTANTS_H
