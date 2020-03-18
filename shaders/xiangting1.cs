// compute shader
// 判断一向听，依赖公用函数

#version 460 core

layout (local_size_x = 595, local_size_y = 1, local_size_z = 1) in;

const uint SHOUPAI_N = 28;

layout (std430, binding = 0) readonly buffer in_buffer
{
    // 参数
    layout(offset = 0) uint tile_num; // 手牌的总数，最多(SHOUPAI_N - 2)张
    layout(offset = 4) uint fulu_num; // 已有副露（含暗杠）的数目
    layout(offset = 8) uint shoupai[SHOUPAI_N]; // 参数：手牌
    layout(offset = 8 + 4 * SHOUPAI_N) uint paishan[34]; // 参数：牌山剩余牌
};

layout (std430, binding = 1) writeonly buffer out_buffer
{
    // 返回值
    layout(offset = 0) uint out_hule; // 0=没和, 1=和了
    layout(offset = 4) uint out_paixing[1]; // 1=一般, 2=七对, 3=国士
    layout(offset = 8) uint out_hupai[14];
};

const uint TILE_1M = 0x11; // 一萬 🀇
const uint TILE_2M = 0x12; // 二萬 🀈
const uint TILE_3M = 0x13; // 三萬 🀉
const uint TILE_4M = 0x14; // 四萬 🀊
const uint TILE_5M = 0x15; // 五萬 🀋
const uint TILE_6M = 0x16; // 六萬 🀌
const uint TILE_7M = 0x17; // 七萬 🀍
const uint TILE_8M = 0x18; // 八萬 🀎
const uint TILE_9M = 0x19; // 九萬 🀏

const uint TILE_1S = 0x21; // 一索 🀐
const uint TILE_2S = 0x22; // 二索 🀑
const uint TILE_3S = 0x23; // 三索 🀒
const uint TILE_4S = 0x24; // 四索 🀓
const uint TILE_5S = 0x25; // 五索 🀔
const uint TILE_6S = 0x26; // 六索 🀕
const uint TILE_7S = 0x27; // 七索 🀖
const uint TILE_8S = 0x28; // 八索 🀗
const uint TILE_9S = 0x29; // 九索 🀘

const uint TILE_1P = 0x31; // 一筒 🀙
const uint TILE_2P = 0x32; // 二筒 🀚
const uint TILE_3P = 0x33; // 三筒 🀛
const uint TILE_4P = 0x34; // 四筒 🀜
const uint TILE_5P = 0x35; // 五筒 🀝
const uint TILE_6P = 0x36; // 六筒 🀞
const uint TILE_7P = 0x37; // 七筒 🀟
const uint TILE_8P = 0x38; // 八筒 🀠
const uint TILE_9P = 0x39; // 九筒 🀡

const uint TILE_1Z = 0x41; // 東 🀀
const uint TILE_2Z = 0x49; // 南 🀁
const uint TILE_3Z = 0x51; // 西 🀂
const uint TILE_4Z = 0x59; // 北 🀃
const uint TILE_5Z = 0x61; // 白 🀆
const uint TILE_6Z = 0x69; // 發 🀅
const uint TILE_7Z = 0x71; // 中 🀄

const uint TILE_0M = 0x95; // 赤五萬 🀋
const uint TILE_0S = 0xA5; // 赤五索 🀔
const uint TILE_0P = 0xB5; // 赤五筒 🀝

const uint THE_TILES[34] =
{
    TILE_1M, TILE_2M, TILE_3M,
    TILE_4M, TILE_5M, TILE_6M,
    TILE_7M, TILE_8M, TILE_9M,

    TILE_1S, TILE_2S, TILE_3S,
    TILE_4S, TILE_5S, TILE_6S,
    TILE_7S, TILE_8S, TILE_9S,

    TILE_1P, TILE_2P, TILE_3P,
    TILE_4P, TILE_5P, TILE_6P,
    TILE_7P, TILE_8P, TILE_9P,

    TILE_1Z, TILE_2Z, TILE_3Z, TILE_4Z,

    TILE_5Z, TILE_6Z, TILE_7Z,
};

uint has_hule(uint tile_n, uint fulu_n, const uint tiles[SHOUPAI_N],
    out uint out_paixing[1], out uint out_hupai[14]);

const uint COMB_R2[34] = // COMB_R2[k + 1] = COMB_R2[k] + k + 2
{
    1  , 3  , 6  , 10 , 15 , 21 , 28 , 36 ,
    45 , 55 , 66 , 78 , 91 , 105, 120, 136,
    153, 171, 190, 210, 231, 253, 276, 300,
    325, 351, 378, 406, 435, 465, 496, 528,
    561, 595,
};

shared uint min_index = COMB_R2[33];

void main()
{
    uint paixing[1] = { 0 };
    uint hupai[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };
    uint hule = 0;

    const uint x = gl_LocalInvocationIndex;

    if (x == 0)
    {
        out_hule = 0;
    }

    uint x0 = x;
    uint x1 = 0;
    for (x1 = 0; x1 < 34; x1++)
    {
        if (x0 < COMB_R2[x1])
        {
            if (x1 > 0)
            {
                x0 -= COMB_R2[x1 - 1];
            }
            break;
        }
    }

    bool enough = true;

    uint remain = paishan[x0];
    if (remain > 0)
    {
        remain--;
    }
    else
    {
        enough = false;
    }

    if (x1 > x0)
    {
        remain = paishan[x1];
    }
    if (remain > 0)
    {
        remain--;
    }
    else
    {
        enough = false;
    }

    if (enough)
    {
        const uint TILE_ADD = 2;
        uint mopai[TILE_ADD] =
        {
            THE_TILES[x0],
            THE_TILES[x1],
        };
        uint tiles[SHOUPAI_N];
        uint old_i = 0;
        uint add_i = 0;
        for (uint i = 0; i < tile_num + TILE_ADD; i++)
        {
            if (add_i < TILE_ADD)
            {
                if (old_i < tile_num)
                {
                    if (mopai[add_i] < shoupai[old_i])
                    {
                        tiles[i] = mopai[add_i];
                        add_i++;
                    }
                    else
                    {
                        tiles[i] = shoupai[old_i];
                        old_i++;
                    }
                }
                else
                {
                    tiles[i] = mopai[add_i];
                    add_i++;
                }
            }
            else
            {
                tiles[i] = shoupai[old_i];
                old_i++;
            }
        }

        hule = has_hule (tile_num + TILE_ADD, 0, tiles, paixing, hupai);

        if (hule > 0)
        {
            atomicMin (min_index, x);
        }
    }

    // memoryBarrierShared ();
    barrier ();

    if (min_index == x)
    {
        out_hule = hule;
        out_paixing = paixing;
        out_hupai = hupai;
    }

    return;
}

// end of file
