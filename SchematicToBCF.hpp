
#pragma once  
#include "BCFCachedWriter.hpp"  
#include <nbt_tags.h>  
#include <io/stream_reader.h>  
#include <io/izlibstream.h>  
#include <io/ozlibstream.h>  
#include <iostream>    
#include <sstream>    
#include <fstream>  
#include <vector>  
#include <string>  
#include <unordered_map>  

class SchematicToBCF {
private:
    const std::string m_filename;
    const std::string m_outputFilename;

    // 保持原有的 blockIdToName 映射表  
    std::unordered_map<int, std::string> blockIdToName = {
       {151, "daylight_detector"},
    {541, "chain"},
    {505, "crimson_standing_sign"},
    {208, "grass_path"},
    {411, "sea_pickle"},
    {260, "stripped_spruce_log"},
    {180, "red_sandstone_stairs"},
    {547, "polished_blackstone_stairs"},
    {171, "carpet"},
    {176, "standing_banner"},
    {200, "chorus_flower"},
    {133, "emerald_block"},
    {13, "gravel"},
    {549, "polished_blackstone_double_slab"},
    {175, "double_plant"},
    {154, "hopper"},
    {153, "quartz_ore"},
    {129, "emerald_ore"},
    {186, "dark_oak_fence_gate"},
    {558, "cracked_nether_bricks"},
    {398, "jungle_button"},
    {31, "tallgrass"},
    {533, "polished_blackstone_brick_wall"},
    {493, "nether_sprouts"},
    {441, "birch_standing_sign"},
    {12, "sand"},
    {266, "blue_ice"},
    {69, "lever"},
    {158, "wooden_slab"},
    {252, "concrete_powder"},
    {85, "fence"},
    {511, "crimson_fence"},
    {25, "noteblock"},
    {178, "daylight_detector_inverted"},
    {506, "warped_standing_sign"},
    {434, "mossy_cobblestone_stairs"},
    {81, "cactus"},
    {248, "green_glazed_terracotta"},
    {15, "iron_ore"},
    {553, "warped_hyphae"},
    {521, "crimson_double_slab"},
    {250, "black_glazed_terracotta"},
    {487, "crimson_nylium"},
    {9, "water"},
    {482, "warped_wart_block"},
    {138, "beacon"},
    {147, "light_weighted_pressure_plate"},
    {501, "crimson_trapdoor"},
    {18, "leaves"},
    {32, "deadbush"},
    {202, "colored_torch_rg"},
    {150, "powered_comparator"},
    {120, "end_portal_frame"},
    {179, "red_sandstone"},
    {394, "dried_kelp_block"},
    {225, "pink_shulker_box"},
    {531, "blackstone_stairs"},
    {475, "honey_block"},
    {554, "crimson_hyphae"},
    {407, "dark_oak_pressure_plate"},
    {195, "jungle_door"},
    {462, "sweet_berry_bush"},
    {420, "scaffolding"},
    {492, "soul_fire"},
    {209, "end_gateway"},
    {242, "gray_glazed_terracotta"},
    {7, "bedrock"},
    {212, "frosted_ice"},
    {470, "light_block"},
    {93, "unpowered_repeater"},
    {489, "basalt"},
    {502, "warped_trapdoor"},
    {20, "glass"},
    {536, "gilded_blackstone"},
    {46, "tnt"},
    {126, "wooden_slab"},
    {485, "shroomlight"},
    {247, "brown_glazed_terracotta"},
    {480, "crimson_stem"},
    {34, "piston_head"},
    {177, "wall_banner"},
    {56, "diamond_ore"},
    {516, "warped_button"},
    {125, "dropper"},
    {83, "reeds"},
    {477, "lodestone"},
    {392, "coral_fan_hang3"},
    {391, "coral_fan_hang2"},
    {72, "wooden_pressure_plate"},
    {29, "sticky_piston"},
    {10, "flowing_lava"},
    {198, "end_rod"},
    {230, "blue_shulker_box"},
    {24, "sandstone"},
    {527, "respawn_anchor"},
    {456, "fletching_table"},
    {61, "furnace"},
    {114, "nether_brick_stairs"},
    {246, "blue_glazed_terracotta"},
    {65, "ladder"},
    {468, "composter"},
    {237, "magenta_glazed_terracotta"},
    {448, "darkoak_wall_sign"},
    {52, "mob_spawner"},
    {430, "mossy_stone_brick_stairs"},
    {451, "blast_furnace"},
    {168, "prismarine"},
    {190, "hard_glass_pane"},
    {402, "dark_oak_trapdoor"},
    {474, "beehive"},
    {526, "ancient_debris"},
    {41, "gold_block"},
    {476, "honeycomb_block"},
    {87, "netherrack"},
    {243, "light_gray_glazed_terracotta"},
    {108, "brick_stairs"},
    {64, "wooden_door"},
    {146, "trapped_chest"},
    {121, "end_stone"},
    {429, "polished_andesite_stairs"},
    {431, "smooth_red_sandstone_stairs"},
    {557, "chiseled_nether_bricks"},
    {396, "birch_button"},
    {513, "crimson_fence_gate"},
    {6, "sapling"},
    {75, "unlit_redstone_torch"},
    {181, "double_stone_slab2"},
    {194, "birch_door"},
    {424, "granite_stairs"},
    {216, "bone_block"},
    {39, "brown_mushroom"},
    {481, "warped_stem"},
    {471, "wither_rose"},
    {155, "quartz_block"},
    {44, "stone_slab"},
    {142, "potatoes"},
    {116, "enchanting_table"},
    {463, "lantern"},
    {201, "purpur_block"},
    {170, "hay_block"},
    {50, "torch"},
    {535, "cracked_polished_blackstone_bricks"},
    {8, "flowing_water"},
    {540, "polished_blackstone_brick_double_slab"},
    {387, "coral_block"},
    {515, "crimson_button"},
    {217, "structure_void"},
    {205, "undyed_shulker_box"},
    {91, "lit_pumpkin"},
    {539, "polished_blackstone_brick_slab"},
    {101, "iron_bars"},
    {261, "stripped_birch_log"},
    {401, "birch_trapdoor"},
    {162, "log2"},
    {488, "warped_nylium"},
    {416, "honey_block"},
    {3, "dirt"},
    {73, "redstone_ore"},
    {410, "carved_pumpkin"},
    {218, "shulker_box"},
    {98, "stonebrick"},
    {433, "end_brick_stairs"},
    {60, "farmland"},
    {245, "purple_glazed_terracotta"},
    {214, "nether_wart_block"},
    {84, "jukebox"},
    {265, "stripped_oak_log"},
    {77, "stone_button"},
    {38, "red_flower"},
    {452, "stonecutter_block"},
    {514, "warped_fence_gate"},
    {143, "wooden_button"},
    {119, "end_portal"},
    {109, "stone_brick_stairs"},
    {509, "crimson_stairs"},
    {512, "warped_fence"},
    {409, "spruce_pressure_plate"},
    {43, "double_stone_slab"},
    {465, "lava_cauldron"},
    {442, "birch_wall_sign"},
    {135, "birch_stairs"},
    {486, "weeping_vines"},
    {495, "stripped_crimson_stem"},
    {412, "conduit"},
    {483, "crimson_fungus"},
    {544, "crying_obsidian"},
    {518, "warped_pressure_plate"},
    {232, "green_shulker_box"},
    {517, "crimson_pressure_plate"},
    {415, "bubble_column"},
    {508, "warped_wall_sign"},
    {70, "stone_pressure_plate"},
    {494, "target"},
    {440, "smooth_quartz_stairs"},
    {26, "bed"},
    {559, "quartz_bricks"},
    {23, "dispenser"},
    {236, "orange_glazed_terracotta"},
    {141, "carrots"},
    {219, "white_shulker_box"},
    {395, "acacia_button"},
    {231, "brown_shulker_box"},
    {545, "soul_campfire"},
    {182, "stone_slab2"},
    {63, "standing_sign"},
    {88, "soul_sand"},
    {105, "melon_stem"},
    {172, "hardened_clay"},
    {80, "snow"},
    {54, "chest"},
    {107, "fence_gate"},
    {131, "tripwire_hook"},
    {525, "netherite_block"},
    {406, "birch_pressure_plate"},
    {210, "repeating_command_block"},
    {42, "iron_block"},
    {21, "lapis_ore"},
    {257, "prismarine_stairs"},
    {388, "coral_fan"},
    {234, "black_shulker_box"},
    {159, "stained_hardened_clay"},
    {211, "chain_command_block"},
    {40, "red_mushroom"},
    {49, "obsidian"},
    {543, "nether_gold_ore"},
    {458, "barrel"},
    {484, "warped_fungus"},
    {5, "planks"},
    {189, "birch_fence"},
    {524, "soul_lantern"},
    {400, "acacia_trapdoor"},
    {519, "crimson_slab"},
    {145, "anvil"},
    {467, "wood"},
    {397, "skull"},
    {35, "wool"},
    {86, "pumpkin"},
    {499, "crimson_door"},
    {187, "acacia_fence_gate"},
    {241, "pink_glazed_terracotta"},
    {532, "blackstone_wall"},
    {529, "polished_blackstone_bricks"},
    {148, "heavy_weighted_pressure_plate"},
    {405, "acacia_pressure_plate"},
    {408, "jungle_pressure_plate"},
    {498, "warped_planks"},
    {251, "concrete"},
    {199, "frame"},
    {106, "vine"},
    {227, "silver_shulker_box"},
    {226, "gray_shulker_box"},
    {390, "coral_fan_hang"},
    {167, "iron_trapdoor"},
    {111, "waterlily"},
    {403, "jungle_trapdoor"},
    {55, "redstone_wire"},
    {469, "lit_blast_furnace"},
    {33, "piston"},
    {548, "polished_blackstone_slab"},
    {534, "chiseled_polished_blackstone"},
    {196, "acacia_door"},
    {169, "seaLantern"},
    {89, "glowstone"},
    {233, "red_shulker_box"},
    {207, "beetroot"},
    {67, "stone_stairs"},
    {223, "yellow_shulker_box"},
    {92, "cake"},
    {204, "colored_torch_bp"},
    {57, "diamond_block"},
    {262, "stripped_jungle_log"},
    {510, "warped_stairs"},
    {164, "dark_oak_stairs"},
    {450, "grindstone"},
    {461, "bell"},
    {115, "nether_wart"},
    {264, "stripped_dark_oak_log"},
    {220, "orange_shulker_box"},
    {244, "cyan_glazed_terracotta"},
    {449, "lectern"},
    {96, "trapdoor"},
    {97, "monster_egg"},
    {490, "polished_basalt"},
    {117, "brewing_stand"},
    {546, "polished_blackstone"},
    {426, "end_crystal"},
    {14, "gold_ore"},
    {393, "kelp"},
    {542, "twisting_vines"},
    {51, "fire"},
    {99, "brown_mushroom_block"},
    {132, "tripWire"},
    {530, "polished_blackstone_brick_stairs"},
    {122, "dragon_egg"},
    {389, "coral_fan_dead"},
    {160, "stained_glass_pane"},
    {459, "loom"},
    {36, "element_0"},
    {258, "dark_prismarine_stairs"},
    {500, "warped_door"},
    {0, "air"},
    {134, "spruce_stairs"},
    {444, "jungle_wall_sign"},
    {427, "polished_granite_stairs"},
    {491, "soul_soil"},
    {496, "stripped_warped_stem"},
    {428, "polished_diorite_stairs"},
    {188, "spruce_fence"},
    {68, "wall_sign"},
    {76, "redstone_torch"},
    {11, "lava"},
    {137, "command_block"},
    {66, "rail"},
    {206, "end_bricks"},
    {90, "portal"},
    {259, "prismarine_bricks_stairs"},
    {556, "stripped_warped_hyphae"},
    {385, "seagrass"},
    {453, "smoker"},
    {161, "leaves2"},
    {552, "polished_blackstone_wall"},
    {425, "diorite_stairs"},
    {551, "polished_blackstone_button"},
    {235, "white_glazed_terracotta"},
    {136, "jungle_stairs"},
    {253, "hard_glass"},
    {165, "slime"},
    {538, "blackstone_double_slab"},
    {193, "spruce_door"},
    {130, "ender_chest"},
    {30, "web"},
    {404, "spruce_trapdoor"},
    {263, "stripped_acacia_log"},
    {537, "blackstone_slab"},
    {139, "cobblestone_wall"},
    {445, "acacia_standing_sign"},
    {228, "cyan_shulker_box"},
    {473, "bee_nest"},
    {128, "sandstone_stairs"},
    {443, "jungle_standing_sign"},
    {102, "glass_pane"},
    {62, "lit_furnace"},
    {555, "stripped_crimson_hyphae"},
    {446, "acacia_wall_sign"},
    {197, "dark_oak_door"},
    {82, "clay"},
    {418, "bamboo"},
    {123, "redstone_lamp"},
    {100, "red_mushroom_block"},
    {37, "yellow_flower"},
    {19, "sponge"},
    {95, "stained_glass"},
    {213, "magma"},
    {550, "polished_blackstone_pressure_plate"},
    {249, "red_glazed_terracotta"},
    {156, "quartz_stairs"},
    {240, "lime_glazed_terracotta"},
    {528, "blackstone"},
    {447, "darkoak_standing_sign"},
    {79, "ice"},
    {437, "spruce_wall_sign"},
    {464, "campfire"},
    {27, "golden_rail"},
    {215, "red_nether_brick"},
    {59, "wheat"},
    {124, "lit_redstone_lamp"},
    {414, "turtle_egg"},
    {497, "crimson_planks"},
    {174, "packed_ice"},
    {1, "stone"},
    {454, "lit_smoker"},
    {399, "spruce_button"},
    {386, "coral"},
    {507, "crimson_wall_sign"},
    {157, "double_wooden_slab"},
    {184, "birch_fence_gate"},
    {523, "soul_torch"},
    {191, "hard_stained_glass_pane"},
    {163, "acacia_stairs"},
    {45, "brick_block"},
    {118, "cauldron"},
    {127, "cocoa"},
    {183, "spruce_fence_gate"},
    {478, "crimson_roots"},
    {239, "yellow_glazed_terracotta"},
    {17, "log"},
    {466, "jigsaw"},
    {71, "iron_door"},
    {173, "coal_block"},
    {520, "warped_slab"},
    {104, "pumpkin_stem"},
    {455, "cartography_table"},
    {185, "jungle_fence_gate"},
    {110, "mycelium"},
    {192, "acacia_fence"},
    {457, "smithing_table"},
    {28, "detector_rail"},
    {436, "spruce_standing_sign"},
    {47, "bookshelf"},
    {254, "hard_stained_glass"},
    {144, "skull"},
    {224, "lime_shulker_box"},
    {229, "purple_shulker_box"},
    {113, "nether_brick_fence"},
    {2, "grass"},
    {203, "purpur_stairs"},
    {472, "stickyPistonArmCollision"},
    {522, "warped_double_slab"},
    {16, "coal_ore"},
    {140, "flower_pot"},
    {94, "powered_repeater"},
    {255, "structure_block"},
    {22, "lapis_block"},
    {149, "unpowered_comparator"},
    {435, "normal_stone_stairs"},
    {103, "melon_block"},
    {419, "bamboo_sapling"},
    {479, "warped_roots"},
    {222, "light_blue_shulker_box"},
    {432, "smooth_sandstone_stairs"},
    {48, "mossy_cobblestone"},
    {4, "cobblestone"},
    {53, "oak_stairs"},
    {152, "redstone_block"},
    {58, "crafting_table"},
    {74, "lit_redstone_ore"},
    {112, "nether_brick"},
    {238, "light_blue_glazed_terracotta"},
    {78, "snow_layer"},
    {438, "smooth_stone"},
    {166, "barrier"},
    {221, "magenta_shulker_box"},
    {295, "wheat_seeds"},
    {321, "painting"},
    {354, "cake"},
    {355, "bed"},
    };

public:
    SchematicToBCF(const std::string& filename, const std::string& outputFilename)
        : m_filename(filename)
        , m_outputFilename(outputFilename)
    {
        convert();
    }

    void convert() {
        std::ifstream file(m_filename, std::ios::binary);
        if (!file) {
            std::cerr << "无法打开文件" << std::endl;
            return;
        }

        // 优化 3: 增加内存阈值到 50000  
        BCFCachedWriter writer(m_outputFilename, "./temp_bcf_cache", 50000);

        try {
            // 读取并解压缩文件  
            zlib::izlibstream gzstream(file);
            auto [name, schematic] = nbt::io::read_compound(gzstream);

            // 获取尺寸  
            int16_t width = static_cast<int16_t>(schematic->at("Width"));
            int16_t height = static_cast<int16_t>(schematic->at("Height"));
            int16_t length = static_cast<int16_t>(schematic->at("Length"));

            // 读取 Blocks 数组  
            auto& blocks = schematic->at("Blocks").as<nbt::tag_byte_array>();
            std::cout << "方块数组大小: " << blocks.size() << std::endl;

            // 读取 Data 数组  
            auto& data = schematic->at("Data").as<nbt::tag_byte_array>();
            std::cout << "数据数组大小: " << data.size() << std::endl;

            // 优化 2: 预先构建空气方块过滤器  
            std::vector<bool> isAirBlock(256, false);
            isAirBlock[0] = true;  // 空气方块 ID  

            // 优化 4: 预先构建方块名称缓存  
            std::vector<std::string> blockNameCache(256);
            for (const auto& [id, name] : blockIdToName) {
                if (id >= 0 && id < 256) {
                    blockNameCache[id] = name;
                }
            }

            // 优化 1: 预分配状态向量  
            std::vector<std::pair<std::string, std::string>> states;
            states.reserve(1);

            // 预先缓存 "tileData" 字符串  
            const std::string tileDataKey = "tileData";

            // 遍历所有方块 (保持 YZX 顺序以提高空间局部性)  
            for (int y = 0; y < height; ++y) {
                for (int z = 0; z < length; ++z) {
                    for (int x = 0; x < width; ++x) {
                        // Schematic 使用 YZX 顺序存储  
                        int index = x + (z * width) + (y * width * length);

                        // 获取方块 ID 和数据值  
                        int8_t blockId = blocks[index];
                        int8_t blockData = data[index];

                        // 优化 2: 使用预构建的空气过滤器  
                        if (blockId >= 0 && blockId < 256 && !isAirBlock[blockId]) {
                            // 优化 4: 使用预构建的方块名称缓存  
                            const std::string& blockName = blockNameCache[blockId];

                            if (blockName.empty()) {
                                continue;  // 跳过未知方块  
                            }

                            // 优化 1: 重用状态向量,使用 move 语义  
                            states.clear();
                            states.emplace_back(tileDataKey, std::to_string(blockData));
                            writer.addBlock(x, y, z, blockName, std::move(states));
                        }
                    }
                }
            }

            std::cout << "转换完成!" << std::endl;
            writer.finalize();
            std::cout << "写入文件完成!" << std::endl;
        }
        catch (const nbt::io::input_error& e) {
            std::cerr << "读取错误: " << e.what() << std::endl;
            return;
        }
        catch (const std::bad_cast& e) {
            std::cerr << "类型转换错误: " << e.what() << std::endl;
            return;
        }
    }
};
