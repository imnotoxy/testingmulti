// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_REPORT_HPP
#define SC_REPORT_HPP

#include <fstream>
#include <iostream>
#include <array>

#include "sc_highchart.hpp"
#include "sc_enums.hpp"
#include "util/io.hpp"

struct player_t;
struct xml_node_t;
struct sc_timeline_t;
struct spell_data_t;
class extended_sample_data_t;
struct player_processed_report_information_t;
struct sim_report_information_t;
struct spell_data_expr_t;
class reforge_plot_run_t;
struct plot_data_t;

#include <chrono>
/**
 * Automatic Timer reporting the time between construction and desctruction of the object.
 */
struct Timer
{
private:
  std::string title;
  std::ostream& out;
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
public:
  Timer(std::string title, std::ostream& out = std::cout) :
      title(std::move(title)),
      out(out),
      start(std::chrono::high_resolution_clock::now())
  {

  }
  ~Timer()
  {
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = end - start;
    typedef std::chrono::duration<double> float_seconds;
    out << "Generating " << title << " took " << std::chrono::duration_cast<float_seconds>(diff).count() << "seconds." << std::endl;
  }
};

#define MAX_PLAYERS_PER_CHART 20

#define LOOTRANK_ENABLED 0 // The website works, but the link we send out is not usable. If anyone ever fixes it, just set this to 1.

namespace chart
{
enum chart_e { HORIZONTAL_BAR_STACKED, HORIZONTAL_BAR, VERTICAL_BAR, PIE, LINE, XY_LINE };

std::string raid_downtime ( const std::vector<player_t*> &players_by_name );
size_t raid_aps ( std::vector<std::string>& images, const sim_t&, const std::vector<player_t*>&, std::string type );
size_t raid_dpet( std::vector<std::string>& images, const sim_t& );
size_t raid_gear( std::vector<std::string>& images, const sim_t& );

std::string action_dpet        ( const player_t& );
std::string aps_portion        ( const player_t& );
std::string time_spent         ( const player_t& );
std::string gains              ( const player_t&, resource_e );
std::string timeline           ( const std::vector<double>&, const std::string&, double avg = 0, std::string color = "FDD017", size_t max_length = 0 );
std::string timeline_dps_error ( const player_t& );
std::string scale_factors      ( const player_t& );
std::string scaling_dps        ( const player_t& );
std::vector<std::pair<unsigned,std::string>> reforge_dps        ( const player_t& );
std::string distribution       ( const std::vector<size_t>& /*dist_data*/, const std::string&, double, double, double );
std::string normal_distribution(  double mean, double std_dev, double confidence, double tolerance_interval = 0  );
std::string dps_error( player_t& );

std::array<std::string, SCALE_METRIC_MAX> gear_weights_lootrank  ( const player_t& );
std::array<std::string, SCALE_METRIC_MAX> gear_weights_wowhead   ( const player_t& );
std::array<std::string, SCALE_METRIC_MAX> gear_weights_pawn      ( const player_t& );
std::array<std::string, SCALE_METRIC_MAX> gear_weights_askmrrobot( const player_t& );

// Highcharts stuff
bool generate_raid_gear( highchart::bar_chart_t&, const sim_t& );
bool generate_raid_downtime( highchart::bar_chart_t&, const sim_t& );
bool generate_raid_aps( highchart::bar_chart_t&, const sim_t&, const std::string& type );
bool generate_distribution( highchart::histogram_chart_t&, const player_t* p,
                                 const std::vector<size_t>& dist_data,
                                 const std::string& distribution_name,
                                 double avg, double min, double max );
bool generate_gains( highchart::pie_chart_t&, const player_t&, resource_e );
bool generate_spent_time( highchart::pie_chart_t&, const player_t& );
bool generate_stats_sources( highchart::pie_chart_t&, const player_t&, const std::string title, const std::vector<stats_t*>& stats_list );
bool generate_damage_stats_sources( highchart::pie_chart_t&, const player_t& );
bool generate_heal_stats_sources( highchart::pie_chart_t&, const player_t& );
bool generate_raid_dpet( highchart::bar_chart_t&, const sim_t& );
bool generate_action_dpet( highchart::bar_chart_t&, const player_t& );
bool generate_apet( highchart::bar_chart_t&, const std::vector<stats_t*>& );
highchart::time_series_t& generate_stats_timeline( highchart::time_series_t&, const stats_t& );
highchart::time_series_t& generate_actor_timeline( highchart::time_series_t&,
                                                   const player_t&      p,
                                                   const std::string&   attribute,
                                                   const std::string&   series_color,
                                                   const sc_timeline_t& data );
bool generate_actor_dps_series( highchart::time_series_t& series, const player_t& p );
bool generate_scale_factors( highchart::bar_chart_t& bc, const player_t& p, scale_metric_e metric );
bool generate_scaling_plot( highchart::chart_t& bc, const player_t& p, scale_metric_e metric );
bool generate_reforge_plot( highchart::chart_t& bc, const player_t& p, const std::pair<const reforge_plot_run_t*,std::vector<std::vector<plot_data_t>>>& plr  );
 
} // end namespace sc_chart

namespace color
{
struct rgb
{
  unsigned char r_, g_, b_;
  double a_;

  rgb();

  rgb( unsigned char r, unsigned char g, unsigned char b );
  rgb( double r, double g, double b );
  rgb( const std::string& color );
  rgb( const char* color );

  std::string rgb_str() const;
  std::string str() const;
  std::string hex_str() const;

  rgb& adjust( double v );
  rgb adjust( double v ) const;
  rgb dark( double pct = 0.25 ) const;
  rgb light( double pct = 0.25 ) const;
  rgb opacity( double pct = 1.0 ) const;

  rgb& operator=( const std::string& color_str );
  rgb& operator+=( const rgb& other );
  rgb operator+( const rgb& other ) const;
  operator std::string() const;

private:
  bool parse_color( const std::string& color_str );
};

std::ostream& operator<<( std::ostream& s, const rgb& r );

rgb mix( const rgb& c0, const rgb& c1 );

rgb class_color( player_e type );
rgb resource_color( resource_e type );
rgb stat_color( stat_e type );
rgb school_color( school_e school );

// Class colors
const rgb COLOR_DEATH_KNIGHT = "C41F3B";
const rgb COLOR_DRUID        = "FF7D0A";
const rgb COLOR_HUNTER       = "ABD473";
const rgb COLOR_MAGE         = "69CCF0";
const rgb COLOR_MONK         = "00FF96";
const rgb COLOR_PALADIN      = "F58CBA";
const rgb COLOR_PRIEST       = "FFFFFF";
const rgb COLOR_ROGUE        = "FFF569";
const rgb COLOR_SHAMAN       = "0070DE";
const rgb COLOR_WARLOCK      = "9482C9";
const rgb COLOR_WARRIOR      = "C79C6E";

const rgb WHITE              = "FFFFFF";
const rgb GREY               = "333333";
const rgb GREY2              = "666666";
const rgb GREY3              = "8A8A8A";
const rgb YELLOW             = COLOR_ROGUE;
const rgb PURPLE             = "9482C9";
const rgb RED                = COLOR_DEATH_KNIGHT;
const rgb TEAL               = "009090";
const rgb BLACK              = "000000";

// School colors
const rgb COLOR_NONE         = WHITE;
const rgb PHYSICAL           = COLOR_WARRIOR;
const rgb HOLY               = "FFCC00";
const rgb FIRE               = COLOR_DEATH_KNIGHT;
const rgb NATURE             = COLOR_HUNTER;
const rgb FROST              = COLOR_SHAMAN;
const rgb SHADOW             = PURPLE;
const rgb ARCANE             = COLOR_MAGE;
const rgb ELEMENTAL          = COLOR_MONK;
const rgb FROSTFIRE          = "9900CC";

// Item quality colors
const rgb POOR               = "9D9D9D";
const rgb COMMON             = WHITE;
const rgb UNCOMMON           = "1EFF00";
const rgb RARE               = "0070DD";
const rgb EPIC               = "A335EE";
const rgb LEGENDARY          = "FF8000";
const rgb HEIRLOOM           = "E6CC80";

} /* namespace color */

namespace report
{

typedef io::ofstream sc_html_stream;

void generate_player_charts         ( player_t&, player_processed_report_information_t& );
void generate_player_buff_lists     ( player_t&, player_processed_report_information_t& );
void generate_sim_report_information( const sim_t&, sim_report_information_t& );

void print_html_sample_data ( report::sc_html_stream&, const player_t&, const extended_sample_data_t&, const std::string& name, int& td_counter, int columns = 1 );

bool output_scale_factors( const player_t* p );

void print_spell_query ( std::ostream& out, const sim_t& sim, const spell_data_expr_t&, unsigned level );
void print_spell_query ( xml_node_t* out, FILE* file, const sim_t& sim, const spell_data_expr_t&, unsigned level );
void print_profiles    ( sim_t* );
void print_text        ( sim_t*, bool detail );
void print_html        ( sim_t& );
void print_json        ( sim_t& );
void print_html_player ( report::sc_html_stream&, player_t&, int );
void print_xml         ( sim_t* );
void print_suite       ( sim_t* );

const color::rgb& item_quality_color( const item_t& item );

std::string decoration_domain( const sim_t& sim );
std::string decorated_buff_name( const buff_t* buff );
std::string decorated_action_name( const action_t* action );
std::string decorated_spell_name( const sim_t& sim, const spell_data_t& spell );
std::string decorated_item_name( const item_t* item );

#if SC_BETA
static const char* const beta_warnings[] =
{
  "Beta! Beta! Beta! Beta! Beta! Beta!",
  "Not All classes are yet supported.",
  "Some class models still need tweaking.",
  "Some class action lists need tweaking.",
  "Some class BiS gear setups need tweaking.",
  "Some trinkets not yet implemented.",
  "Constructive feedback regarding our output will shorten the Beta phase dramatically.",
  "Beta! Beta! Beta! Beta! Beta! Beta!",
};
#endif // SC_BETA

std::array<std::string, SCALE_METRIC_MAX> gear_weights_lootrank  ( player_t* );
std::array<std::string, SCALE_METRIC_MAX> gear_weights_wowhead   ( player_t* );
std::array<std::string, SCALE_METRIC_MAX> gear_weights_askmrrobot( player_t* );
std::array<std::string, SCALE_METRIC_MAX> gear_weights_pawn( player_t* p );

std::string decorate_html_string( const std::string& value, const color::rgb& color );
} // reort

std::string pretty_spell_text( const spell_data_t& default_spell, const std::string& text, const player_t& p );
inline std::string pretty_spell_text( const spell_data_t& default_spell, const char* text, const player_t& p )
{ return text ? pretty_spell_text( default_spell, std::string( text ), p ) : std::string(); }

#endif // SC_REPORT_HPP
