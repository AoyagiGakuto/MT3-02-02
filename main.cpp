#include <Novice.h>
#include <cmath>
#include <corecrt_math.h>
#include <cstdint>
#include <imgui.h>

const char kWindowTitle[] = "LE2D_02_アオヤギ_ガクト_確認課題";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

struct Vector3 {
    float x, y, z;
};

struct Matrix4x4 {
    float m[4][4];
};

struct Sphere {
    Vector3 center;
    float radius;
};

struct Plane {
    Vector3 point; // 平面上の1点
    Vector3 normal; // 法線
};

// ベクトルのドット積
float Dot(const Vector3& a, const Vector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// ベクトル正規化
Vector3 Normalize(const Vector3& v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0001f) {
        return { v.x / len, v.y / len, v.z / len };
    }
    return v;
}

Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b)
{
    Matrix4x4 r {};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                r.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return r;
}

Matrix4x4 MakeViewProjectionMatrix(const Vector3& cameraTranslate, const Vector3& cameraRotate)
{
    float cosY = cosf(cameraRotate.y);
    float sinY = sinf(cameraRotate.y);
    float cosX = cosf(cameraRotate.x);
    float sinX = sinf(cameraRotate.x);
    float cosZ = cosf(cameraRotate.z);
    float sinZ = sinf(cameraRotate.z);

    Matrix4x4 rotY {};
    rotY.m[0][0] = cosY;
    rotY.m[0][2] = sinY;
    rotY.m[1][1] = 1.0f;
    rotY.m[2][0] = -sinY;
    rotY.m[2][2] = cosY;
    rotY.m[3][3] = 1.0f;

    Matrix4x4 rotX {};
    rotX.m[0][0] = 1.0f;
    rotX.m[1][1] = cosX;
    rotX.m[1][2] = -sinX;
    rotX.m[2][1] = sinX;
    rotX.m[2][2] = cosX;
    rotX.m[3][3] = 1.0f;

    Matrix4x4 rotZ {};
    rotZ.m[0][0] = cosZ;
    rotZ.m[0][1] = -sinZ;
    rotZ.m[1][0] = sinZ;
    rotZ.m[1][1] = cosZ;
    rotZ.m[2][2] = 1.0f;
    rotZ.m[3][3] = 1.0f;

    Matrix4x4 rot = Multiply(Multiply(rotZ, rotX), rotY);

    Matrix4x4 trans {};
    trans.m[0][0] = 1.0f;
    trans.m[1][1] = 1.0f;
    trans.m[2][2] = 1.0f;
    trans.m[3][3] = 1.0f;
    trans.m[3][0] = -cameraTranslate.x;
    trans.m[3][1] = -cameraTranslate.y;
    trans.m[3][2] = -cameraTranslate.z;

    Matrix4x4 view = Multiply(trans, rot);

    Matrix4x4 proj {};
    float fovY = 60.0f * (M_PI / 180.0f);
    float aspect = 1280.0f / 720.0f;
    float nearZ = 0.1f;
    float farZ = 100.0f;
    float f = 1.0f / tanf(fovY / 2.0f);

    proj.m[0][0] = f / aspect;
    proj.m[1][1] = f;
    proj.m[2][2] = farZ / (farZ - nearZ);
    proj.m[2][3] = (-nearZ * farZ) / (farZ - nearZ);
    proj.m[3][2] = 1.0f;
    proj.m[3][3] = 0.0f;

    return Multiply(view, proj);
}

Matrix4x4 MakeViewportForMatrix(float left, float top, float width, float height, float minDepth, float maxDepth)
{
    Matrix4x4 m {};
    m.m[0][0] = width * 0.5f;
    m.m[1][1] = -height * 0.5f;
    m.m[2][2] = (maxDepth - minDepth);
    m.m[3][0] = left + width * 0.5f;
    m.m[3][1] = top + height * 0.5f;
    m.m[3][2] = minDepth;
    m.m[3][3] = 1.0f;
    return m;
}

Vector3 Transform(const Vector3& v, const Matrix4x4& m)
{
    float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    if (w != 0.0f) {
        x /= w;
        y /= w;
        z /= w;
    }
    return { x, y, z };
}

void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix)
{
    const float kGridHalfWidth = 2.0f;
    const uint32_t kSubdivision = 10;
    const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

    // X方向（縦線）
    for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
        float x = -kGridHalfWidth + xIndex * kGridEvery;
        Vector3 start = { x, 0.0f, -kGridHalfWidth };
        Vector3 end = { x, 0.0f, kGridHalfWidth };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }

    // Z方向（横線）
    for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
        float z = -kGridHalfWidth + zIndex * kGridEvery;
        Vector3 start = { -kGridHalfWidth, 0.0f, z };
        Vector3 end = { kGridHalfWidth, 0.0f, z };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }
}

// 球の描画
void DrawSphere(const Sphere& sphere, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color)
{
    const uint32_t kSubdivision = 16;
    const float kLatEvery = M_PI / float(kSubdivision);
    const float kLonEvery = 2.0f * M_PI / float(kSubdivision);

    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        float lat = -M_PI / 2.0f + kLatEvery * latIndex;
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            float lon = lonIndex * kLonEvery;

            Vector3 a {
                sphere.center.x + sphere.radius * cosf(lat) * cosf(lon),
                sphere.center.y + sphere.radius * sinf(lat),
                sphere.center.z + sphere.radius * cosf(lat) * sinf(lon)
            };
            Vector3 b {
                sphere.center.x + sphere.radius * cosf(lat + kLatEvery) * cosf(lon),
                sphere.center.y + sphere.radius * sinf(lat + kLatEvery),
                sphere.center.z + sphere.radius * cosf(lat + kLatEvery) * sinf(lon)
            };
            Vector3 c {
                sphere.center.x + sphere.radius * cosf(lat) * cosf(lon + kLonEvery),
                sphere.center.y + sphere.radius * sinf(lat),
                sphere.center.z + sphere.radius * cosf(lat) * sinf(lon + kLonEvery)
            };

            a = Transform(a, viewProjectionMatrix);
            a = Transform(a, viewportMatrix);
            b = Transform(b, viewProjectionMatrix);
            b = Transform(b, viewportMatrix);
            c = Transform(c, viewProjectionMatrix);
            c = Transform(c, viewportMatrix);

            Novice::DrawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, color);
            Novice::DrawLine((int)a.x, (int)a.y, (int)c.x, (int)c.y, color);
        }
    }
}

// 平面を簡単に描画する（XZ方向に四角形）
void DrawPlane(const Plane& plane, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color)
{
    float size = 2.0f;
    Vector3 corners[4] = {
        { plane.point.x - size, plane.point.y, plane.point.z - size },
        { plane.point.x + size, plane.point.y, plane.point.z - size },
        { plane.point.x + size, plane.point.y, plane.point.z + size },
        { plane.point.x - size, plane.point.y, plane.point.z + size }
    };

    for (int i = 0; i < 4; i++) {
        Vector3 p1 = Transform(corners[i], viewProjectionMatrix);
        p1 = Transform(p1, viewportMatrix);
        Vector3 p2 = Transform(corners[(i + 1) % 4], viewProjectionMatrix);
        p2 = Transform(p2, viewportMatrix);

        Novice::DrawLine((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, color);
    }
}

// 球と平面の衝突判定
bool IsSpherePlaneCollision(const Sphere& sphere, const Plane& plane)
{
    Vector3 n = Normalize(plane.normal);
    float d = Dot({ sphere.center.x - plane.point.x,
                      sphere.center.y - plane.point.y,
                      sphere.center.z - plane.point.z },
        n);
    return fabsf(d) <= sphere.radius;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, 1280, 720);

    // キー入力結果を受け取る箱
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    Vector3 cameraTranslate { 0.0f, -4.0f, -10.0f };
    Vector3 cameraRotate { -0.2f, 0.0f, 0.0f };

    Sphere sphere = { { 0.0f, 1.0f, 0.0f }, 1.0f };

    Plane plane;
    plane.point = { 0.0f, 0.0f, 0.0f }; // 原点に平面
    plane.normal = { 0.0f, 1.0f, 0.0f }; // 上向きの法線

    Matrix4x4 viewportMatrix = MakeViewportForMatrix(
        0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f);
    Matrix4x4 viewProjectionMatrix = MakeViewProjectionMatrix(cameraTranslate, cameraRotate);

    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0) {
        // フレームの開始
        Novice::BeginFrame();

        // キー入力を受け取る
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ///
        /// ↓更新処理ここから
        ///

        viewProjectionMatrix = MakeViewProjectionMatrix(cameraTranslate, cameraRotate);

        // 衝突判定（球と平面）
        bool isHit = IsSpherePlaneCollision(sphere, plane);

        ///
        /// ↑更新処理ここまで
        ///

        ///
        /// ↓描画処理ここから
        ///

        ImGui::Begin("Control");
        ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
        ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
        ImGui::DragFloat3("Sphere Pos", &sphere.center.x, 0.01f);
        ImGui::DragFloat("Sphere Radius", &sphere.radius, 0.01f);
        ImGui::DragFloat3("Plane Point", &plane.point.x, 0.01f);
        ImGui::DragFloat3("Plane Normal", &plane.normal.x, 0.01f);
        ImGui::Text("Collision: %s", isHit ? "YES" : "NO");
        ImGui::End();

        DrawGrid(
            viewProjectionMatrix,
            viewportMatrix);

        // 球を衝突してたら赤、してなかったら白
        uint32_t sphereColor = isHit ? RED : WHITE;
        DrawSphere(sphere, viewProjectionMatrix, viewportMatrix, sphereColor);

        // 平面を青で描画
        DrawPlane(plane, viewProjectionMatrix, viewportMatrix, BLUE);

        ///
        /// ↑描画処理ここまで
        ///

        // フレームの終了
        Novice::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    // ライブラリの終了
    Novice::Finalize();
    return 0;
}