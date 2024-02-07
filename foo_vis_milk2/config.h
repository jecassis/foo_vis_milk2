#pragma once

#include <vis_milk2/plugin.h>

class milk2_config
{
  public:
    milk2_config();

    uint32_t g_get_version() const;
    uint32_t get_sentinel() const;
    void reset();
    void parse(ui_element_config_parser& parser);
    void build(ui_element_config_builder& builder);

    // `milk2.ini`
    plugin_config settings;

  private:
    uint32_t m_sentinel = ('M' << 24 | 'I' << 16 | 'L' << 8 | 'K');
    uint32_t m_version = 1u;

    static void initialize_paths();
    void update_paths();
};