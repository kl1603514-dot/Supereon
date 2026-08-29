#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <tchar.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <process.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "sat_prism.h"
// #include "sat_prism.c"
#include "settings.h"
#include "self_update.h"

// ============ OFFSETS ============
#define OFFSET_HUMANOID_HEALTH 0x190
#define OFFSET_HUMANOID_JUMP 0x1da
#define OFFSET_HUMANOID_STATE 0x8c0
#define OFFSET_HUMANOID_STATEID 0x20
#define OFFSET_PLAYER_TEAM 0x2d8
#define OFFSET_BASEPART_TRANSPARENCY 0x130
#define OFFSET_DATAMODEL_WORKSPACE 0x158
#define OFFSET_PLAYER_CHARACTER 0x298
#define OFFSET_INSTANCE_CHILDREN 0x78
#define OFFSET_INSTANCE_CHILDREN_END 0x8
#define OFFSET_INSTANCE_PARENT 0x68
#define OFFSET_INSTANCE_NAME_CONTAINER 0x70
#define OFFSET_INSTANCE_CLASS_BASE 0x18
#define OFFSET_BASEPART_PRIMITIVE 0x188
#define OFFSET_PRIMITIVE_POSITION 0xec
#define OFFSET_PRIMITIVE_ROTATION 0xc8
#define OFFSET_PRIMITIVE_SIZE 0x1bc
#define OFFSET_PRIMITIVE_VELOCITY 0xf8
#define OFFSET_FAKE_DATAMODEL 0x8ca9cc8
#define OFFSET_REAL_DATAMODEL 0x1f8
#define OFFSET_STRING_LENGTH 0x10
#define OFFSET_STRING_DATA 0x0
// ============ END OFFSETS ============

const float PixelPerRadians = 540;
const double MouseSensitivity = 0.45;
const double EPSILON = 1e-9;
const float Step_Theta = 0.075;
const char *targetName = "RobloxPlayerBeta.exe";

HANDLE hProc = NULL;
static HHOOK mouseHook = NULL;

// ===== HIDDEN BLOAT - 2MB of data =====
static volatile unsigned char __bloat_data[2097152] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
    0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
    0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
    0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
    0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
    0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
    0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
    0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

static void __keep_bloat(void) {
    volatile unsigned char* ptr = __bloat_data;
    volatile unsigned char val = ptr[0];
    val = ptr[1024];
    val = ptr[2048];
    val = ptr[4096];
    val = ptr[8192];
    val = ptr[16384];
    val = ptr[32768];
    val = ptr[65536];
    val = ptr[131072];
    val = ptr[262144];
    val = ptr[524288];
    val = ptr[1048576];
    val = ptr[2097151];
    (void)val;
}

// ===== HOTKEY HANDLER =====
LRESULT CALLBACK HookKeysProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        int KeyCode = pKeyboard->vkCode;
        if (KeyCode == VK_F12 && wParam == WM_KEYDOWN) {
            g_cfg.running = 0;
            PostQuitMessage(0);
        }
        if (KeyCode == VK_F4 && wParam == WM_KEYDOWN) {
            g_cfg.jumpbot = !g_cfg.jumpbot;
        }
        if (KeyCode == VK_F5 && wParam == WM_KEYDOWN) {
            g_cfg.aimbot = !g_cfg.aimbot;
        }
        if (KeyCode == VK_F7 && wParam == WM_KEYDOWN) {
            g_cfg.teamcheck = !g_cfg.teamcheck;
        }
        if (KeyCode == VK_F8 && wParam == WM_KEYDOWN) {
            g_cfg.duels = !g_cfg.duels;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        return 1;
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void BlockMouseInput(void) {
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
}

void UnblockMouseInput(void) {
    if (mouseHook) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = NULL;
    }
}

// ===== MEMORY FUNCTIONS =====
DWORD FindProcessId(const char *processName) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;
    if (Process32First(hSnap, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, processName) == 0) {
                DWORD pid = pe32.th32ProcessID;
                CloseHandle(hSnap);
                return pid;
            }
        } while (Process32Next(hSnap, &pe32));
    }
    CloseHandle(hSnap);
    return 0;
}

uint64_t get_base_address(HANDLE hProc){
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (!EnumProcessModules(hProc, hMods, sizeof(hMods), &cbNeeded)) {
        CloseHandle(hProc);
        return 0;
    }
    unsigned int moduleCount = cbNeeded / sizeof(HMODULE);
    for (unsigned int i = 0; i < moduleCount; i++) {
        char modName[MAX_PATH];
        if (GetModuleFileNameExA(hProc, hMods[i], modName, sizeof(modName))) {
            const char *p = strrchr(modName, '\\');
            const char *base = (p ? p+1 : modName);
            if (_stricmp(base, targetName) == 0) {
                return (uint64_t)hMods[i];
            }
        }
    }
    return 0;
}

bool readMemory(uint64_t targetAddress, void* buffer, SIZE_T size) {
    SIZE_T bytesRead = 0;
    if (ReadProcessMemory(hProc, (LPCVOID)targetAddress, buffer, size, &bytesRead)) {
        return bytesRead == size;
    }
    return false; 
}

char *get_string(uint64_t address) {
    int length;
    if (!readMemory(address + OFFSET_STRING_LENGTH, &length, sizeof(int))) {
        return NULL;
    }
    if (length <= 0 || length > 1024) return NULL;
    char *string = malloc(length + 1);
    if (!string) return NULL;
    if (length < 16) {
        if (!readMemory(address + OFFSET_STRING_DATA, string, length)) {
            free(string);
            return NULL;
        }
    } else {
        uint64_t new_address;
        if (!readMemory(address + OFFSET_STRING_DATA, &new_address, sizeof(uint64_t)) ||
            !readMemory(new_address, string, length)) {
            free(string);
            return NULL;
        }
    }
    string[length] = '\0';
    return string;
}

char *get_name(uint64_t instance) {
    uint64_t name_container;
    if (!readMemory(instance + OFFSET_INSTANCE_NAME_CONTAINER, &name_container, sizeof(uint64_t))) {
        return NULL;
    }
    return get_string(name_container + 0x8);
}

char *get_class(uint64_t instance) {
    uint64_t class_base;
    if (!readMemory(instance + OFFSET_INSTANCE_CLASS_BASE, &class_base, sizeof(uint64_t))) {
        return NULL;
    }
    uint64_t class_address;
    if (!readMemory(class_base + 0x8, &class_address, sizeof(uint64_t))) {
        return NULL;
    }
    return get_string(class_address);
}

char *get_team(uint64_t player_address) { 
    uint64_t team_address;
    if (!readMemory(player_address + OFFSET_PLAYER_TEAM, &team_address, sizeof(uint64_t))) {
        return NULL;
    }
    if (team_address == 0) return NULL;
    return get_name(team_address);
}

uint64_t find_first_child(uint64_t Instance, const char *Name) {
    if (Instance == 0) return 0;
    uint64_t START;
    if (!readMemory(Instance + OFFSET_INSTANCE_CHILDREN, &START, sizeof(uint64_t))) return 0;
    uint64_t end_child;
    if (!readMemory(START + OFFSET_INSTANCE_CHILDREN_END, &end_child, sizeof(uint64_t))) return 0;
    uint64_t start_child;
    if (!readMemory(START, &start_child, sizeof(uint64_t))) return 0;
    for (uint64_t childOffset = start_child; childOffset != end_child; childOffset += 2 * sizeof(uint64_t)) {
        uint64_t child;
        if (!readMemory(childOffset, &child, sizeof(uint64_t))) break;
        char *childName = get_name(child);
        if (childName) {
            if (strcmp(childName, Name) == 0) {
                free(childName);
                return child;
            }
            free(childName);
        }
    }
    return 0;
}

uint64_t find_first_class(uint64_t Instance, const char *Name) {
    if (Instance == 0) return 0;
    uint64_t START;
    if (!readMemory(Instance + OFFSET_INSTANCE_CHILDREN, &START, sizeof(uint64_t))) return 0;
    uint64_t end_child;
    if (!readMemory(START + OFFSET_INSTANCE_CHILDREN_END, &end_child, sizeof(uint64_t))) return 0;
    uint64_t start_child;
    if (!readMemory(START, &start_child, sizeof(uint64_t))) return 0;
    for (uint64_t childOffset = start_child; childOffset != end_child; childOffset += 2 * sizeof(uint64_t)) {
        uint64_t child;
        if (!readMemory(childOffset, &child, sizeof(uint64_t))) break;
        char *childClass = get_class(child);
        if (childClass) {
            if (strcmp(childClass, Name) == 0) {
                free(childClass);
                return child;
            }
            free(childClass);
        }
    }
    return 0;
}

uint64_t *get_children(uint64_t Instance, int *OutCount) {
    if (Instance == 0) {
        *OutCount = 0;
        return NULL;
    }
    uint64_t START;
    if (!readMemory(Instance + OFFSET_INSTANCE_CHILDREN, &START, sizeof(uint64_t))) return NULL;
    uint64_t end_child;
    if (!readMemory(START + OFFSET_INSTANCE_CHILDREN_END, &end_child, sizeof(uint64_t))) return NULL;
    uint64_t start_child;
    if (!readMemory(START, &start_child, sizeof(uint64_t))) return NULL;
    int count = (int)((end_child - start_child) / (2 * sizeof(uint64_t)));
    if (count <= 0) {
        *OutCount = 0;
        return NULL;
    }
    uint64_t *children = malloc(sizeof(uint64_t) * count);
    if (!children) {
        *OutCount = 0;
        return NULL;
    }
    for (int i = 0; i < count; i++) {
        uint64_t child;
        readMemory(start_child + 2 * i * sizeof(uint64_t), &child, sizeof(uint64_t));
        children[i] = child;
    }
    *OutCount = count;
    return children;
}

Vec3 get_position(uint64_t Address) {
    Vec3 V3Position = {0,0,0};
    if (Address == 0) return V3Position;
    uint64_t Primitive;
    if (!readMemory(Address + OFFSET_BASEPART_PRIMITIVE, &Primitive, sizeof(uint64_t))) return V3Position;
    if (Primitive == 0) return V3Position;
    if (!readMemory(Primitive + OFFSET_PRIMITIVE_POSITION, &V3Position, sizeof(V3Position))) return V3Position;
    return V3Position;
}

Vec3 get_size(uint64_t Address) {
    Vec3 V3Size = { 0,0,0 };
    if (Address == 0) return V3Size;
    uint64_t Primitive;
    if (!readMemory(Address + OFFSET_BASEPART_PRIMITIVE, &Primitive, sizeof(uint64_t))) return V3Size;
    if (Primitive == 0) return V3Size;
    if (!readMemory(Primitive + OFFSET_PRIMITIVE_SIZE, &V3Size, sizeof(V3Size))) return V3Size;
    return V3Size;
}

Vec3 get_rotation_euler(uint64_t Address) {
    Vec3 V3Rotation = { 0,0,0 };
    if (Address == 0) return V3Rotation;
    uint64_t Primitive;
    if (!readMemory(Address + OFFSET_BASEPART_PRIMITIVE, &Primitive, sizeof(uint64_t))) return V3Rotation;
    if (Primitive == 0) return V3Rotation;
    float Rotation[9];
    if (!readMemory(Primitive + OFFSET_PRIMITIVE_ROTATION, Rotation, sizeof(Rotation))) return V3Rotation;
    Vec3 Look = { Rotation[2], Rotation[5], Rotation[8] };
    Vec3 Up = { Rotation[1], Rotation[4], Rotation[7] };
    Vec3 Right = { Rotation[0], Rotation[3], Rotation[6] };
    float pitch = asin(-Look.z);
    float yaw, roll;
    if (fabs(Look.y) < 0.999999) {
        yaw = atan2f(Look.x, Look.z);
        roll = atan2f(Right.y, Up.y);
    } else {
        yaw = atan2f(-Right.z, Right.x);
        roll = 0;
    }
    V3Rotation.x = pitch;
    V3Rotation.y = yaw;
    V3Rotation.z = roll;
    return V3Rotation;
}

void get_rotations(uint64_t Address, Vec3 OutRotation[3]) {
    if (Address == 0) return;
    uint64_t Primitive;
    if (!readMemory(Address + OFFSET_BASEPART_PRIMITIVE, &Primitive, sizeof(uint64_t))) return;
    if (Primitive == 0) return;
    float Rotation[9];
    if (!readMemory(Primitive + OFFSET_PRIMITIVE_ROTATION, Rotation, sizeof(Rotation))) return;
    OutRotation[0] = vec3(Rotation[0], Rotation[3], Rotation[6]);
    OutRotation[1] = vec3(Rotation[1], Rotation[4], Rotation[7]);
    OutRotation[2] = vec3(Rotation[2], Rotation[5], Rotation[8]);
}

Vec3 get_look_vector(uint64_t Address) {
    Vec3 look = {0, 0, 0};
    if (Address == 0) return look;
    uint64_t Primitive;
    if (!readMemory(Address + OFFSET_BASEPART_PRIMITIVE, &Primitive, sizeof(uint64_t))) return look;
    if (Primitive == 0) return look;
    float Rotation[9];
    if (!readMemory(Primitive + OFFSET_PRIMITIVE_ROTATION, Rotation, sizeof(Rotation))) return look;
    look.x = Rotation[2];
    look.y = Rotation[5];
    look.z = Rotation[8];
    return look;
}

float get_magnitude(Vec3 V3) {
    return sqrtf(V3.x*V3.x + V3.y*V3.y + V3.z*V3.z);
}

float get_angle_between(Vec3 v1, Vec3 v2) {
    float dot = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    float mag1 = get_magnitude(v1);
    float mag2 = get_magnitude(v2);
    if (mag1 < EPSILON || mag2 < EPSILON) return 180.0f;
    float cos_angle = dot / (mag1 * mag2);
    cos_angle = fmax(-1.0f, fmin(1.0f, cos_angle));
    return acosf(cos_angle) * 180.0f / M_PI;
}

uint64_t get_char(uint64_t PlayerAddress) {
    if (PlayerAddress == 0) return 0;
    uint64_t CharacterAddress;
    if (!readMemory(PlayerAddress + OFFSET_PLAYER_CHARACTER, &CharacterAddress, sizeof(uint64_t))) {
        return 0;
    }
    return CharacterAddress;
}

Vec3 get_distance(Vec3 V3_1, Vec3 V3_2) {
    Vec3 Distance;
    Distance.x = V3_1.x - V3_2.x;
    Distance.y = V3_1.y - V3_2.y;
    Distance.z = V3_1.z - V3_2.z;
    return Distance;
}

Vertices get_vertices(uint64_t BasePart) {
    Vertices OutVertices = {0};
    if (BasePart == 0) return OutVertices;
    Vec3 Base_rotations[3] = {0};
    get_rotations(BasePart, Base_rotations);
    Vec3 XVector = Base_rotations[0];
    Vec3 YVector = Base_rotations[1];
    Vec3 ZVector = Base_rotations[2];
    Vec3 Position = get_position(BasePart);
    Vec3 Size = get_size(BasePart);
    Vec3 hx = vec3_mul(XVector, Size.x * 0.5f);
    Vec3 hy = vec3_mul(YVector, Size.y * 0.5f);
    Vec3 hz = vec3_mul(ZVector, Size.z * 0.5f);
    OutVertices.vec[0] = vec3_sub(vec3_sub(vec3_sub(Position, hx), hy), hz);
    OutVertices.vec[1] = vec3_sub(vec3_sub(vec3_add(Position, hx), hy), hz);
    OutVertices.vec[2] = vec3_sub(vec3_add(vec3_add(Position, hx), hy), hz);
    OutVertices.vec[3] = vec3_sub(vec3_add(vec3_sub(Position, hx), hy), hz);
    OutVertices.vec[4] = vec3_add(vec3_sub(vec3_sub(Position, hx), hy), hz);
    OutVertices.vec[5] = vec3_add(vec3_sub(vec3_add(Position, hx), hy), hz);
    OutVertices.vec[6] = vec3_add(vec3_add(vec3_add(Position, hx), hy), hz);
    OutVertices.vec[7] = vec3_add(vec3_add(vec3_sub(Position, hx), hy), hz);
    return OutVertices;
}

Vertices get_closesthandle_vertices(uint64_t TargetTorso, uint64_t closestSwordHandle) {
    Vertices OutVertices = {0};
    if (TargetTorso == 0 || closestSwordHandle == 0) return OutVertices;
    Vec3 Base_rotations[3] = {0};
    Vec3 Torso_position = get_position(TargetTorso);
    get_rotations(TargetTorso, Base_rotations);
    Vec3 TorsoCF[4] = {0};
    TorsoCF[0] = Torso_position;
    TorsoCF[1] = Base_rotations[0];
    TorsoCF[2] = Base_rotations[1];
    TorsoCF[3] = Base_rotations[2];
    Vec3 handle_rotations[3] = {0};
    Vec3 Sword_position = get_position(closestSwordHandle);
    get_rotations(closestSwordHandle, handle_rotations);
    Vec3 SwordCF[4] = {0};
    SwordCF[0] = Sword_position;
    SwordCF[1] = handle_rotations[0];
    SwordCF[2] = handle_rotations[1];
    SwordCF[3] = handle_rotations[2];
    Vec3 Position = SwordCF[0];
    Vec3 XVector = SwordCF[1];
    Vec3 YVector = SwordCF[2];
    Vec3 ZVector = SwordCF[3];
    Vec3 Size = get_size(closestSwordHandle);
    Vec3 hx = vec3_mul(XVector, Size.x * 0.5f);
    Vec3 hy = vec3_mul(YVector, Size.y * 0.5f);
    Vec3 hz = vec3_mul(ZVector, Size.z * 0.5f);
    OutVertices.vec[0] = vec3_sub(vec3_sub(vec3_sub(Position, hx), hy), hz);
    OutVertices.vec[1] = vec3_sub(vec3_sub(vec3_add(Position, hx), hy), hz);
    OutVertices.vec[2] = vec3_sub(vec3_add(vec3_add(Position, hx), hy), hz);
    OutVertices.vec[3] = vec3_sub(vec3_add(vec3_sub(Position, hx), hy), hz);
    OutVertices.vec[4] = vec3_add(vec3_sub(vec3_sub(Position, hx), hy), hz);
    OutVertices.vec[5] = vec3_add(vec3_sub(vec3_add(Position, hx), hy), hz);
    OutVertices.vec[6] = vec3_add(vec3_add(vec3_add(Position, hx), hy), hz);
    OutVertices.vec[7] = vec3_add(vec3_add(vec3_sub(Position, hx), hy), hz);
    return OutVertices;
}

Vertices rotate_vertices(Vertices Input, Vec3 Center, float theta) {
    Vertices Output = {0};
    float c = cosf(theta);
    float s = sinf(theta);
    for (int i = 0; i < 8; i++) {
        Vec3 p;
        p.x = Input.vec[i].x - Center.x;
        p.y = Input.vec[i].y - Center.y;
        p.z = Input.vec[i].z - Center.z;
        Vec3 r;
        r.x = p.x * c + p.z * s;
        r.y = p.y;
        r.z = -p.x * s + p.z * c;
        Output.vec[i].x = r.x + Center.x;
        Output.vec[i].y = r.y + Center.y;
        Output.vec[i].z = r.z + Center.z;
    }
    return Output;
}

Vec3 get_velocity(uint64_t Address) {
    Vec3 velocity = {0,0,0};
    if (Address == 0) return velocity;
    uint64_t Primitive_ptr;
    if (!readMemory(Address + OFFSET_BASEPART_PRIMITIVE, &Primitive_ptr, sizeof(uint64_t))) return velocity;
    if (Primitive_ptr == 0) return velocity;
    readMemory(Primitive_ptr + OFFSET_PRIMITIVE_VELOCITY, &velocity, sizeof(velocity));
    return velocity;
}

void pressSpace() {
    static DWORD lastJumpTime = 0;
    if (GetTickCount() - lastJumpTime < 500) return;
    UINT scanCode = MapVirtualKey(VK_SPACE, MAPVK_VK_TO_VSC);
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = (WORD)scanCode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &input, sizeof(INPUT));
    Sleep(50);
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
    lastJumpTime = GetTickCount();
}

void moveMouseRel(int dx, int dy) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
}

// JumpSyncState structure
typedef struct {
    uint64_t playerAddr;
    bool isSynced;
    bool wasAtRest;
    bool isWaitingForRest;
    float lastVelocity;
} JumpSyncState;

// ===== WORKER THREAD =====
static unsigned __stdcall worker_thread(void *arg) {
    (void)arg;
    __keep_bloat();
    
    static JumpSyncState syncStates[64];
    static int syncStateCount = 0;
    bool firstRun = true;
    
    while (g_cfg.running) {
        DWORD pid = FindProcessId(targetName);
        if (pid == 0) {
            if (firstRun) {
                firstRun = false;
            }
            Sleep(1000);
            continue;
        }

        hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProc == NULL) {
            Sleep(1000);
            continue;
        }

        uint64_t Base = get_base_address(hProc);
        if (Base == 0) {
            CloseHandle(hProc);
            Sleep(1000);
            continue;
        }

        uint64_t fake_datamodel;
        if (!readMemory(Base + OFFSET_FAKE_DATAMODEL, &fake_datamodel, sizeof(uint64_t))) {
            CloseHandle(hProc);
            Sleep(1000);
            continue;
        }

        uint64_t datamodel;
        if (!readMemory(fake_datamodel + OFFSET_REAL_DATAMODEL, &datamodel, sizeof(uint64_t))) {
            CloseHandle(hProc);
            Sleep(1000);
            continue;
        }
        uint64_t WorkspaceAddress = find_first_child(datamodel, "Workspace");
        uint64_t PlayersAddress = find_first_class(datamodel, "Players");
        if (PlayersAddress == 0) {
            CloseHandle(hProc);
            Sleep(1000);
            continue;
        }
        
        int Playercount = 0;
        uint64_t *Players = get_children(PlayersAddress, &Playercount);
        if (!Players || Playercount <= 0) {
            if (Players) free(Players);
            Sleep(500);
            continue;
        }

        uint64_t LocalCharacter = get_char(Players[0]);
        if (LocalCharacter == 0) {
            free(Players);
            Sleep(500);
            continue;
        }

        uint64_t LocalHRP = find_first_child(LocalCharacter, "HumanoidRootPart");
        if (LocalHRP == 0) {
            free(Players);
            Sleep(500);
            continue;
        }
        
        uint64_t SwordTool = find_first_class(LocalCharacter, "Tool");
        uint64_t SwordHandle = 0;
        if (SwordTool != 0) {
            SwordHandle = find_first_child(SwordTool, "Handle");
        }

        Vec3 LocalPosition = get_position(LocalHRP);
        uint64_t LocalHum = find_first_child(LocalCharacter, "Humanoid");
        if (LocalHum == 0) {
            free(Players);
            Sleep(500);
            continue;
        }
        
        float LocalHealth = 0;
        readMemory(LocalHum + OFFSET_HUMANOID_HEALTH, &LocalHealth, sizeof(float));
        
        float closestMagnitude = INFINITY;
        Vec3 closestDistance = {0,0,0};
        char *LocalTeam = get_team(Players[0]);
        uint64_t closestPlayer = 0;
        Vec3 LocalLook = get_look_vector(LocalHRP);
        
        for (int i = 1; i < Playercount; i++) {
            uint64_t OtherCharacter = get_char(Players[i]);
            if (!OtherCharacter) continue;
            
            uint64_t OtherHRP = find_first_child(OtherCharacter, "Torso");
            if (!OtherHRP) OtherHRP = find_first_child(OtherCharacter, "HumanoidRootPart");
            if (!OtherHRP) continue;
            
            uint64_t OtherHum = find_first_child(OtherCharacter, "Humanoid");
            if (!OtherHum) continue;
            
            char *OtherTeam = get_team(Players[i]);
            float OtherHealth = 0;
            readMemory(OtherHum + OFFSET_HUMANOID_HEALTH, &OtherHealth, sizeof(float));
            
            Vec3 OtherPosition = get_position(OtherHRP);
            Vec3 distanceVec3 = get_distance(LocalPosition, OtherPosition);
            
            float Transparency = 0;
            readMemory(OtherHRP + OFFSET_BASEPART_TRANSPARENCY, &Transparency, sizeof(float));

            if(fabsf(distanceVec3.y) > 3 || Transparency == 1) {
                if (OtherTeam) free(OtherTeam);
                continue;
            }

            if (g_cfg.teamcheck) {
                if (LocalTeam == NULL || OtherTeam == NULL) {
                    if (OtherTeam) free(OtherTeam);
                    continue;
                }
                if (strcmp(LocalTeam, OtherTeam) == 0) {
                    if (OtherTeam) free(OtherTeam);
                    continue;
                }
            }

            if (g_cfg.duels) {
                uint64_t Highlight = find_first_class(OtherCharacter, "Highlight");
                if (Highlight == 0) {
                    if (OtherTeam) free(OtherTeam);
                    continue;
                }
            }
            
            distanceVec3.y = 0;
            float Magnitude = get_magnitude(distanceVec3);
            
            Vec3 dirToEnemy = distanceVec3;
            float mag = get_magnitude(dirToEnemy);
            if (mag > EPSILON) {
                dirToEnemy.x /= mag;
                dirToEnemy.y /= mag;
                dirToEnemy.z /= mag;
            }
            
            float dotProduct = LocalLook.x * dirToEnemy.x + LocalLook.y * dirToEnemy.y + LocalLook.z * dirToEnemy.z;
            float angleToEnemy = get_angle_between(LocalLook, dirToEnemy);

            bool fovPassed = !g_cfg.fov_check || (dotProduct > 0 && angleToEnemy <= 179.0f);
            if (closestMagnitude > Magnitude && OtherHealth > 0 && OtherHealth <= 200 && LocalHealth > 0 && fovPassed) {
                closestMagnitude = Magnitude;
                closestDistance = distanceVec3;
                closestPlayer = Players[i];
            }
            
            if (OtherTeam) free(OtherTeam);
        }
        
        if (LocalTeam) free(LocalTeam);

        // Jumpbot code
        if (g_cfg.jumpbot && closestPlayer != 0) {
            static DWORD lastJumpTime = 0;
            DWORD currentTime = GetTickCount();
            
            if (currentTime - lastJumpTime < 150) {
                free(Players);
                Players = NULL;
                Sleep(2);
                continue;
            }
            
            uint64_t LocalTeamAddr = 0;
            readMemory(Players[0] + OFFSET_PLAYER_TEAM, &LocalTeamAddr, sizeof(uint64_t));

            for (int i = 0; i < Playercount; i++) {
                if (Players[i] == Players[0]) continue;
                
                if (g_cfg.teamcheck && LocalTeamAddr != 0) {
                    uint64_t OtherTeamAddr = 0;
                    readMemory(Players[i] + OFFSET_PLAYER_TEAM, &OtherTeamAddr, sizeof(uint64_t));
                    if (OtherTeamAddr == 0) continue;
                    if (OtherTeamAddr == LocalTeamAddr) continue;
                }

                uint64_t OtherChar = get_char(Players[i]);
                if (!OtherChar) continue;

                if (g_cfg.duels) {
                    uint64_t Highlight = find_first_class(OtherChar, "Highlight");
                    if (Highlight == 0) continue;
                }

                uint64_t OtherHum = find_first_child(OtherChar, "Humanoid");
                if (!OtherHum) continue;

                float health = 0;
                if (readMemory(OtherHum + OFFSET_HUMANOID_HEALTH, &health, sizeof(float))) {
                    if (health > 200.0f || health <= 0.0f) continue;
                }

                uint64_t OtherTorso = find_first_child(OtherChar, "Torso");
                if (!OtherTorso) OtherTorso = find_first_child(OtherChar, "HumanoidRootPart");
                if (!OtherTorso) continue;

                float transparency = 0.0f;
                if (readMemory(OtherTorso + OFFSET_BASEPART_TRANSPARENCY, &transparency, sizeof(float))) {
                    if (transparency > 0.95f) continue;
                }

                Vec3 OtherPos = get_position(OtherTorso);
                Vec3 distVec = get_distance(LocalPosition, OtherPos);
                float mag = get_magnitude(distVec);

                if (mag >= g_cfg.jump_dist) continue;

                Vec3 dirToTarget = distVec;
                float dirMag = get_magnitude(dirToTarget);
                if (dirMag > EPSILON) {
                    dirToTarget.x /= dirMag;
                    dirToTarget.y /= dirMag;
                    dirToTarget.z /= dirMag;
                }
                
                float dotProduct = LocalLook.x * dirToTarget.x + LocalLook.y * dirToTarget.y + LocalLook.z * dirToTarget.z;
                float angleToTarget = get_angle_between(LocalLook, dirToTarget);
                
                bool fovPassed = !g_cfg.fov_check || (dotProduct > 0 && angleToTarget <= 179.0f);
                if (!fovPassed) continue;

                Vec3 enemyVelocity = get_velocity(OtherTorso);
                float verticalVelocity = enemyVelocity.y;
                
                JumpSyncState* state = NULL;
                for (int s = 0; s < syncStateCount; s++) {
                    if (syncStates[s].playerAddr == Players[i]) {
                        state = &syncStates[s];
                        break;
                    }
                }
                
                if (state == NULL && syncStateCount < 64) {
                    state = &syncStates[syncStateCount++];
                    state->playerAddr = Players[i];
                    state->isSynced = false;
                    state->wasAtRest = false;
                    state->isWaitingForRest = false;
                    state->lastVelocity = 0.0f;
                }
                
                if (state != NULL) {
                    bool isAtRest = (fabs(verticalVelocity) < 0.5f);
                    
                    if (!state->isSynced && !state->isWaitingForRest) {
                        if (isAtRest) {
                            state->isSynced = true;
                            state->wasAtRest = true;
                            state->lastVelocity = verticalVelocity;
                        } else {
                            state->isWaitingForRest = true;
                            state->wasAtRest = false;
                            state->lastVelocity = verticalVelocity;
                        }
                    }
                    
                    if (state->isWaitingForRest) {
                        if (isAtRest) {
                            state->isWaitingForRest = false;
                            state->isSynced = true;
                            state->wasAtRest = true;
                            state->lastVelocity = verticalVelocity;
                        } else {
                            state->wasAtRest = false;
                            state->lastVelocity = verticalVelocity;
                        }
                    }
                    
                    if (state->isSynced) {
                        bool wasResting = state->wasAtRest;
                        bool velocitySpike = (verticalVelocity > 8.0f && state->lastVelocity < 2.0f && wasResting);
                        bool upwardAccel = (verticalVelocity > 5.0f && state->lastVelocity < verticalVelocity - 3.0f);
                        
                        if (velocitySpike || upwardAccel) {
                            static DWORD jumpTargetTime = 0;
                            static bool isWaitingForJump = false;
                            
                            if (!isWaitingForJump) {
                                jumpTargetTime = currentTime + g_cfg.jump_delay_ms;
                                isWaitingForJump = true;
                            }
                            
                            if (isWaitingForJump && currentTime >= jumpTargetTime) {
                                pressSpace();
                                lastJumpTime = currentTime;
                                isWaitingForJump = false;
                            }
                            
                            state->wasAtRest = false;
                            state->lastVelocity = verticalVelocity;
                            break;
                        }
                        
                        state->wasAtRest = isAtRest;
                        state->lastVelocity = verticalVelocity;
                    }
                }
            }
        }

        // Aimbot code
        if(g_cfg.aimbot && closestPlayer != 0) {
            uint64_t closestCharacter = get_char(closestPlayer);
            if (!closestCharacter) {
                free(Players);
                Players = NULL;
                Sleep(2);
                continue;
            }
            
            uint64_t closestHum = find_first_child(closestCharacter, "Humanoid");
            float closestHealth = 0;
            if (!closestHum || !readMemory(closestHum + OFFSET_HUMANOID_HEALTH, &closestHealth, sizeof(float)) || closestHealth > 200.0f || closestHealth <= 0.0f) {
                free(Players);
                Players = NULL;
                Sleep(2);
                continue;
            }

            uint64_t RightArm = find_first_child(LocalCharacter, "Right Arm");
            uint64_t RightLeg = find_first_child(LocalCharacter, "Right Leg");
            uint64_t LeftArm = find_first_child(LocalCharacter, "Left Arm");
            uint64_t LeftLeg = find_first_child(LocalCharacter, "Left Leg");
            uint64_t Torso = find_first_child(LocalCharacter, "Torso");
            if (!Torso) Torso = find_first_child(LocalCharacter, "UpperTorso");
            uint64_t Head = find_first_child(LocalCharacter, "Head");
            
            if (!RightArm || !RightLeg || !LeftArm || !LeftLeg || !Torso || !Head) {
                free(Players);
                Players = NULL;
                Sleep(2);
                continue;
            }

            Vertices LocalVectrices[7] = {0};
            LocalVectrices[0] = get_vertices(RightArm);
            LocalVectrices[1] = get_vertices(RightLeg);
            LocalVectrices[2] = get_vertices(LeftArm);
            LocalVectrices[3] = get_vertices(LeftLeg);
            LocalVectrices[4] = get_vertices(Torso);
            LocalVectrices[5] = get_vertices(Head);
            LocalVectrices[6] = get_vertices(LocalHRP);

            uint64_t TargetRightArm = find_first_child(closestCharacter, "Right Arm");
            uint64_t TargetRightLeg = find_first_child(closestCharacter, "Right Leg");
            uint64_t TargetLeftArm = find_first_child(closestCharacter, "Left Arm");
            uint64_t TargetLeftLeg = find_first_child(closestCharacter, "Left Leg");
            uint64_t TargetTorso = find_first_child(closestCharacter, "Torso");
            if (!TargetTorso) TargetTorso = find_first_child(closestCharacter, "UpperTorso");
            uint64_t TargetHead = find_first_child(closestCharacter, "Head");
            uint64_t TargetHRP = find_first_child(closestCharacter, "HumanoidRootPart");
            
            if (!TargetRightArm || !TargetRightLeg || !TargetLeftArm || !TargetLeftLeg || !TargetTorso || !TargetHead || !TargetHRP) {
                free(Players);
                Players = NULL;
                Sleep(2);
                continue;
            }

            Vertices OtherVectrices[7] = {0};
            OtherVectrices[0] = get_vertices(TargetRightArm);
            OtherVectrices[1] = get_vertices(TargetRightLeg);
            OtherVectrices[2] = get_vertices(TargetLeftArm);
            OtherVectrices[3] = get_vertices(TargetLeftLeg);
            OtherVectrices[4] = get_vertices(TargetTorso);
            OtherVectrices[5] = get_vertices(TargetHead);
            OtherVectrices[6] = get_vertices(TargetHRP);

            bool OtherTouches[7] = {false};
            bool LocalTouches[7] = {false};

            uint64_t closestSwordTool = find_first_class(closestCharacter, "Tool");
            uint64_t closestSwordHandle = 0;
            Vertices closestHandleVertices = {0};
            if (closestSwordTool) {
                closestSwordHandle = find_first_child(closestSwordTool, "Handle");
            }
            
            if (!closestSwordHandle) {
                memset(&closestHandleVertices, 0, sizeof(Vertices));
            } else {
                closestHandleVertices = get_closesthandle_vertices(TargetTorso, closestSwordHandle);
            }

            if (!SwordHandle) {
                free(Players);
                Players = NULL;
                Sleep(2);
                continue;
            }
            
            // Use reach_margin from settings
            Vertices HandleVertices = inflate_box(get_vertices(SwordHandle), (float)g_cfg.reach_margin);

            for (int i = 0; i < 7; i++) {
                if (closestSwordHandle && PrismsOverlap(closestHandleVertices, LocalVectrices[i]))
                    LocalTouches[i] = true;
                else
                    LocalTouches[i] = false;
            }

            for (int i = 0; i < 7; i++) {
                if (PrismsOverlap(HandleVertices, OtherVectrices[i]))
                    OtherTouches[i] = true;
                else
                    OtherTouches[i] = false;
            }

            bool Temp_OtherTouches[7] = {false};
            Vertices RotatedVertices[8] = {0};

            float Checking_Theta = 0;
            float RightTheta = 0;
            int RightTouches = 0;

            memcpy(Temp_OtherTouches, OtherTouches, sizeof(OtherTouches));

            float range = M_PI / 1.2;
            while (Checking_Theta < range) {
                for (int i = 0; i < 7; i++) {
                    RotatedVertices[i] = rotate_vertices(LocalVectrices[i], LocalPosition, Checking_Theta);
                }
                RotatedVertices[7] = rotate_vertices(HandleVertices, LocalPosition, Checking_Theta);
                bool Touched = false;

                for (int i = 0; i < 7; i++) {
                    if (closestSwordHandle && PrismsOverlap(closestHandleVertices, RotatedVertices[i])) {
                        if (!LocalTouches[i])
                            Touched = true;
                    }
                    else {
                        if (LocalTouches[i])
                            Touched = true;
                    }
                }

                if (Touched)
                    break;

                for (int i = 0; i < 7; i++) {
                    bool isTouch = Temp_OtherTouches[i];

                    if (PrismsOverlap(RotatedVertices[7], OtherVectrices[i])) {
                        Temp_OtherTouches[i] = true;
                    } 
                    else {
                        Temp_OtherTouches[i] = false;
                    }
                    if (Temp_OtherTouches[i] == !isTouch) {
                        RightTheta = Checking_Theta;
                        RightTouches++;
                    }
                }

                Checking_Theta += Step_Theta;
            }

            Checking_Theta = 0;
            float LeftTheta = 0;
            int LeftTouches = 0;

            memcpy(Temp_OtherTouches, OtherTouches, sizeof(OtherTouches));

            while (Checking_Theta > -range) {
                for (int i = 0; i < 7; i++) {
                    RotatedVertices[i] = rotate_vertices(LocalVectrices[i], LocalPosition, Checking_Theta);
                }
                RotatedVertices[7] = rotate_vertices(HandleVertices, LocalPosition, Checking_Theta);
                bool Touched = false;

                for (int i = 0; i < 7; i++) {
                    if (closestSwordHandle && PrismsOverlap(closestHandleVertices, RotatedVertices[i])) {
                        if (!LocalTouches[i])
                            Touched = true;
                    }
                    else {
                        if (LocalTouches[i])
                            Touched = true;
                    }
                }

                if (Touched)
                    break;

                for (int i = 0; i < 7; i++) {
                    bool isTouch = Temp_OtherTouches[i];
                    if (PrismsOverlap(RotatedVertices[7], OtherVectrices[i])) {
                        Temp_OtherTouches[i] = true;
                    } else {
                        Temp_OtherTouches[i] = false;
                    }

                    if (Temp_OtherTouches[i] == !isTouch) {
                        LeftTheta = Checking_Theta;
                        LeftTouches++;
                    }
                }

                Checking_Theta -= Step_Theta;
            }
            
            float delta = -RightTheta;
            if (LeftTouches > RightTouches) {
                delta = -LeftTheta;
            }

            if (delta > 0)
                BlockMouseInput();

            int maxTouches = LeftTouches > RightTouches ? LeftTouches : RightTouches;
            if (maxTouches > 0)
                moveMouseRel((delta * 180 / M_PI * (3 / MouseSensitivity) * (PixelPerRadians / 360)) * g_cfg.wiggle_speed, 0);
        }
        UnblockMouseInput();
        free(Players);
        Players = NULL;
        Sleep(2);
    }

    if (hProc) CloseHandle(hProc);
    return 0;
}

// ===== MAIN =====
extern int run_gui(void); // from gui.c

int main(void) {
    if (self_update_check(__argc, __argv))
        return 0;                 // update downloaded — helper is relaunching us

    __keep_bloat();

    // Set up hotkey hook
    HHOOK hKeysHook = SetWindowsHookEx(WH_KEYBOARD_LL, HookKeysProc, NULL, 0);

    // Start worker thread
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, worker_thread, NULL, 0, NULL);
    if (!hThread) {
        return 1;
    }

    // Run GUI (blocks until window closed)
    run_gui();

    // Signal worker to exit and wait
    g_cfg.running = 0;
    WaitForSingleObject(hThread, 1000);
    CloseHandle(hThread);

    if (hKeysHook) UnhookWindowsHookEx(hKeysHook);
    return 0;
}