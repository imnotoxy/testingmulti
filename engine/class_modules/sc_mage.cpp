// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace residual_action;

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Mage
// ==========================================================================

struct mage_t;

// Forcibly reset mage's current target, if it dies.
struct current_target_reset_cb_t
{
  mage_t* mage;

  current_target_reset_cb_t( player_t* m );

  void operator()(player_t*);
};

struct state_switch_t
{
private:
  bool state;
  timespan_t last_enable,
             last_disable;

public:
  state_switch_t()
  {
    reset();
  }

  bool enable( timespan_t now )
  {
    if ( last_enable == now )
    {
      return false;
    }
    state = true;
    last_enable = now;
    return true;
  }

  bool disable( timespan_t now )
  {
    if ( last_disable == now )
    {
      return false;
    }
    state = false;
    last_disable = now;
    return true;
  }

  bool on()
  {
    return state;
  }

  timespan_t duration( timespan_t now )
  {
    if ( !state )
    {
      return timespan_t::zero();
    }
    return now - last_enable;
  }

  void reset()
  {
    state        = false;
    last_enable  = timespan_t::min();
    last_disable = timespan_t::min();
  }
};

namespace actions {
struct ignite_t;
struct unstable_magic_explosion_t;
} // actions

namespace pets {
enum hero_e
{
  ARTHAS,
  JAINA,
  SYLVANAS,
  TYRANDE
};

struct water_elemental_pet_t;
}

// Icicle data, stored in an icicle container object. Contains a source stats object and the damage
typedef std::pair< double, stats_t* > icicle_data_t;
// Icicle container object, stored in a list to launch icicles at unsuspecting enemies!
typedef std::pair< timespan_t, icicle_data_t > icicle_tuple_t;

struct mage_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* combustion;
    dot_t* flamestrike;
    dot_t* frost_bomb;
    dot_t* ignite;
    dot_t* living_bomb;
    dot_t* nether_tempest;
    dot_t* pyroblast;
    dot_t* frozen_orb;
  } dots;

  struct debuffs_t
  {
    buff_t* slow;
    buff_t* frost_bomb;
    buff_t* firestarter;
    buff_t* water_jet; // Proxy water jet because our expression system sucks
  } debuffs;

  mage_td_t( player_t* target, mage_t* mage );
};

struct mage_t : public player_t
{
public:
  // Current target
  player_t* current_target;

  // Icicles
  std::vector<icicle_tuple_t> icicles;
  action_t* icicle;
  event_t* icicle_event;

  // Active
  actions::ignite_t* active_ignite;
  actions::unstable_magic_explosion_t* unstable_magic_explosion;
  player_t* last_bomb_target;

  // RPPM objects
  real_ppm_t rppm_pyromaniac;         // T17 Fire 4pc
  real_ppm_t rppm_arcane_instability; // T17 Arcane 4pc

  // Tier 18 Arcane pet order tracking
  pets::hero_e last_summoned;

  // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* wild_arcanist; // Arcane
  const special_effect_t* pyrosurge;     // Fire
  const special_effect_t* shatterlance;  // Frost

  // State switches for rotation selection
  state_switch_t burn_phase;
  state_switch_t pyro_chain;

  // Miscellaneous
  double distance_from_rune,
         incanters_flow_stack_mult,
         iv_haste,
         pet_multiplier;

  // Benefits
  struct benefits_t
  {
    benefit_t* arcane_charge[ 4 ]; // CHANGED 2014/4/15 - Arcane Charges max stack is 4 now, not 7.
  } benefits;

  // Buffs
  struct buffs_t
  {
    // Arcane
    buff_t* arcane_charge,
          * arcane_missiles,
          * arcane_power,
          * presence_of_mind,
          * improved_blink,        // Perk
          * profound_magic,        // T16 2pc Arcane
          * arcane_affinity,       // T17 2pc Arcane
          * arcane_instability,    // T17 4pc Arcane
          * temporal_power;        // T18 4pc Arcane

    stat_buff_t* mage_armor;

    // Fire
    buff_t* heating_up,
          * molten_armor,
          * pyroblast,
          * enhanced_pyrotechnics, // Perk
          * improved_scorch,       // Perk
          * fiery_adept,           // T16 4pc Fire
          * pyromaniac,            // T17 4pc Fire
          * icarus_uprising;       // T18 4pc Fire

    stat_buff_t* potent_flames;    // T16 2pc Fire

    // Frost
    buff_t* brain_freeze,
          * fingers_of_frost,
          * frost_armor,
          * icy_veins,
          * enhanced_frostbolt,    // Perk
          * frozen_thoughts,       // T16 2pc Frost
          * ice_shard,             // T17 2pc Frost
          * frost_t17_4pc,         // T17 4pc Frost
          * shatterlance;          // T18 (WoD 6.2) Frost Trinket

    // Talents
    buff_t* blazing_speed,
          * ice_floes,
          * incanters_flow,
          * rune_of_power;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* combustion,
              * cone_of_cold,
              * dragons_breath,
              * frozen_orb,
              * inferno_blast,
              * presence_of_mind;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* evocation;
  } gains;

  // Glyphs
  struct glyphs_t
  {
    // Major
    const spell_data_t* arcane_power;
    const spell_data_t* arcane_explosion;
    const spell_data_t* blink;
    const spell_data_t* combustion;
    const spell_data_t* cone_of_cold;
    const spell_data_t* dragons_breath;
    const spell_data_t* icy_veins;
    const spell_data_t* inferno_blast;
    const spell_data_t* living_bomb;
    const spell_data_t* rapid_displacement;
    const spell_data_t* splitting_ice;
  } glyphs;

  // Passives
  struct passives_t
  {
    const spell_data_t* frost_armor;
    const spell_data_t* mage_armor;
    const spell_data_t* molten_armor;
    const spell_data_t* nether_attunement;
    const spell_data_t* shatter;
  } passives;

  // Perks
  struct perks_t
  {
    //Arcane
    const spell_data_t* enhanced_arcane_blast;
    const spell_data_t* improved_arcane_power;
    const spell_data_t* improved_evocation;
    const spell_data_t* improved_blink;

    //Fire
    const spell_data_t* enhanced_pyrotechnics;
    const spell_data_t* improved_flamestrike;
    const spell_data_t* improved_inferno_blast;
    const spell_data_t* improved_scorch;

    //Frost
    const spell_data_t* enhanced_frostbolt;
    const spell_data_t* improved_blizzard;
    const spell_data_t* improved_water_ele;
    const spell_data_t* improved_icy_veins;
  } perks;

  // Pets
  struct pets_t
  {
    pets::water_elemental_pet_t* water_elemental;
    pet_t* mirror_images[ 3 ];
    pet_t* prismatic_crystal;
    pet_t* temporal_heroes[ 10 ];
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* test_for_crit_hotstreak;
    proc_t* crit_for_hotstreak;
    proc_t* hotstreak;
  } procs;

  // Specializations
  struct specializations_t
  {
    // Arcane
    const spell_data_t* arcane_charge,
    // NOTE: arcane_charge_passive is the Arcane passive added in patch 5.2,
    //       called "Arcane Charge" (Spell ID: 114664).
    //       It contains Spellsteal's mana cost reduction, Evocation's mana
    //       gain reduction, and a deprecated Scorch damage reduction.
                      * arcane_charge_passive,
                      * arcane_mind,
                      * mana_adept;

    // Fire
    const spell_data_t* critical_mass,
                      * ignite,
                      * incineration;

    // Frost
    const spell_data_t* brain_freeze,
                      * fingers_of_frost,
                      * ice_shards,
                      * icicles,
                      * icicles_driver;
  } spec;

  // Talents
  struct talents_list_t
  {
    // Tier 15
    const spell_data_t* evanesce, // NYI
                      * blazing_speed,
                      * ice_floes;
    // Tier 30
    const spell_data_t* alter_time, // NYI
                      * flameglow, // NYI
                      * ice_barrier;
    // Tier 45
    const spell_data_t* ring_of_frost, // NYI
                      * ice_ward, // NYI
                      * frostjaw; //NYI
    // Tier 60
    const spell_data_t* greater_invis, //NYI
                      * cauterize, //NYI
                      * cold_snap;
    // Tier 75
    const spell_data_t* nether_tempest,
                      * living_bomb,
                      * frost_bomb,
                      * unstable_magic,
                      * supernova,
                      * blast_wave,
                      * ice_nova;
    // Tier 90
    const spell_data_t* mirror_image,
                      * rune_of_power,
                      * incanters_flow;
    // Tier 100
    const spell_data_t* overpowered,
                      * kindling,
                      * thermal_void,
                      * prismatic_crystal,
                      * arcane_orb,
                      * meteor,
                      * comet_storm;
  } talents;

public:
  int current_arcane_charges;

  mage_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, MAGE, name, r ),
    current_target( target ),
    icicle( nullptr ),
    icicle_event( nullptr ),
    active_ignite( nullptr ),
    unstable_magic_explosion( nullptr ),
    last_bomb_target( nullptr ),
    rppm_pyromaniac( *this ),
    rppm_arcane_instability( *this ),
    wild_arcanist( nullptr ),
    pyrosurge( nullptr ),
    shatterlance( nullptr ),
    distance_from_rune( 0.0 ),
    iv_haste( 1.0 ),
    pet_multiplier( 1.0 ),
    benefits( benefits_t() ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    glyphs( glyphs_t() ),
    passives( passives_t() ),
    pets( pets_t() ),
    procs( procs_t() ),
    spec( specializations_t() ),
    talents( talents_list_t() ),
    current_arcane_charges()
  {
    // Cooldowns
    cooldowns.combustion         = get_cooldown( "combustion"         );
    cooldowns.cone_of_cold       = get_cooldown( "cone_of_cold"       );
    cooldowns.dragons_breath     = get_cooldown( "dragons_breath"     );
    cooldowns.frozen_orb         = get_cooldown( "frozen_orb"         );
    cooldowns.inferno_blast      = get_cooldown( "inferno_blast"      );
    cooldowns.presence_of_mind   = get_cooldown( "presence_of_mind"   );

    // Miscellaneous
    incanters_flow_stack_mult = find_spell( 116267 ) -> effectN( 1 ).percent();

    // Options
    base.distance = 40;
    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_SPELL_HASTE ] = true;

    // Forcibly reset mage's current target, if it dies.
    struct current_target_reset_cb_t
    {
      mage_t* mage;

      current_target_reset_cb_t( mage_t* m ) : mage( m )
      { }

      void operator()(player_t*)
      {
        for ( size_t i = 0, end = mage -> sim -> target_non_sleeping_list.size(); i < end; ++i )
        {
          // If the mage's current target is still alive, bail out early.
          if ( mage -> current_target == mage -> sim -> target_non_sleeping_list[ i ] )
          {
            return;
          }
        }

        if ( mage -> sim -> debug )
        {
          mage -> sim -> out_debug.printf( "%s current target %s died. Resetting target to %s.",
              mage -> name(), mage -> current_target -> name(), mage -> target -> name() );
        }

        mage -> current_target = mage -> target;
      }
    };
  }

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_benefits() override;
  virtual void      init_stats() override;
  virtual void      reset() override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual action_t* create_proc_action( const std::string& name, const special_effect_t& effect ) override;
  virtual void      create_pets() override;
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e    primary_role() const override { return ROLE_SPELL; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual double    mana_regen_per_second() const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual double    composite_multistrike() const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      update_movement( timespan_t duration ) override;
  virtual void      stun() override;
  virtual double    temporary_movement_modifier() const override;
  virtual void      arise() override;
  virtual action_t* select_action( const action_priority_list_t& ) override;

  target_specific_t<mage_td_t> target_data;

  virtual mage_td_t* get_target_data( player_t* target ) const override
  {
    mage_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new mage_td_t( target, const_cast<mage_t*>(this) );
    }
    return td;
  }

  // Public mage functions:
  icicle_data_t get_icicle_object();
  void trigger_icicle( const action_state_t* trigger_state, bool chain = false, player_t* chain_target = nullptr );

  void              apl_precombat();
  void              apl_arcane();
  void              apl_fire();
  void              apl_frost();
  void              apl_default();
  virtual void      init_action_list() override;

  virtual bool      has_t18_class_trinket() const override;
  std::string       get_potion_action();
};

inline current_target_reset_cb_t::current_target_reset_cb_t( player_t* m ):
  mage( debug_cast<mage_t*>( m ) )
{ }

inline void current_target_reset_cb_t::operator()(player_t*)
{
  for ( size_t i = 0, end = mage -> sim -> target_non_sleeping_list.size(); i < end; ++i )
  {
    // If the mage's current target is still alive, bail out early.
    if ( mage -> current_target == mage -> sim -> target_non_sleeping_list[ i ] )
    {
      return;
    }
  }

  if ( mage -> sim -> debug )
  {
    mage -> sim -> out_debug.printf( "%s current target %s died. Resetting target to %s.",
        mage -> name(), mage -> current_target -> name(), mage -> target -> name() );
  }

  mage -> current_target = mage -> target;
}

namespace pets {

struct mage_pet_spell_t : public spell_t
{
  mage_pet_spell_t( const std::string& n, pet_t* p, const spell_data_t* s ):
    spell_t( n, p, s )
  {}

  mage_t* o()
  {
    pet_t* pet = static_cast< pet_t* >( player );
    mage_t* mage = static_cast< mage_t* >( pet -> owner );
    return mage;
  }

  virtual void schedule_execute( action_state_t* execute_state ) override
  {
    target = o() -> current_target;

    spell_t::schedule_execute( execute_state );
  }
};

struct mage_pet_melee_attack_t : public melee_attack_t
{
  mage_pet_melee_attack_t( const std::string& n, pet_t* p ):
    melee_attack_t( n, p, spell_data_t::nil() )
  {}

  mage_t* o()
  {
    pet_t* pet = static_cast< pet_t* >( player );
    mage_t* mage = static_cast< mage_t* >( pet -> owner );
    return mage;
  }

  virtual void schedule_execute( action_state_t* execute_state ) override
  {
    target = o() -> current_target;

    melee_attack_t::schedule_execute( execute_state );
  }
};

// ==========================================================================
// Pet Water Elemental
// ==========================================================================

struct water_elemental_pet_t;

struct water_elemental_pet_td_t: public actor_target_data_t
{
  buff_t* water_jet;
public:
  water_elemental_pet_td_t( player_t* target, water_elemental_pet_t* welly );
};

struct water_elemental_pet_t : public pet_t
{
  struct freeze_t : public mage_pet_spell_t
  {
    freeze_t( water_elemental_pet_t* p, const std::string& options_str ):
      mage_pet_spell_t( "freeze", p, p -> find_pet_spell( "Freeze" ) )
    {
      parse_options( options_str );
      aoe = -1;
      may_crit = true;
      ignore_false_positive = true;
      action_skill = 1;
    }

    virtual void impact( action_state_t* s ) override
    {
      spell_t::impact( s );

      water_elemental_pet_t* p = static_cast<water_elemental_pet_t*>( player );

      if ( result_is_hit( s -> result ) )
        p -> o() -> buffs.fingers_of_frost
                 -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
    }

  };

  struct water_jet_t : public mage_pet_spell_t
  {
    // queued water jet spell, auto cast water jet spell
    bool queued, autocast;

    water_jet_t( water_elemental_pet_t* p, const std::string& options_str ) :
      mage_pet_spell_t( "water_jet", p, p -> find_spell( 135029 ) ),
      queued( false ), autocast( true )
    {
      parse_options( options_str );
      channeled = tick_may_crit = true;

      if ( p -> o() -> sets.has_set_bonus( MAGE_FROST, T18, B4 ) )
      {
        dot_duration += p -> find_spell( 185971 ) -> effectN( 1 ).time_value();
      }
    }

    water_elemental_pet_td_t* td( player_t* t ) const
    { return p() -> get_target_data( t ? t : target ); }

    water_elemental_pet_t* p()
    { return static_cast<water_elemental_pet_t*>( player ); }

    const water_elemental_pet_t* p() const
    { return static_cast<water_elemental_pet_t*>( player ); }

    void execute() override
    {
      mage_pet_spell_t::execute();

      if ( p() -> o() -> sets.has_set_bonus( MAGE_FROST, T18, B2 ) )
      {
        p() -> o() -> buffs.brain_freeze -> trigger( 1 );
      }

      // If this is a queued execute, disable queued status
      if ( ! autocast && queued )
        queued = false;
    }

    virtual void impact( action_state_t* s ) override
    {
      mage_pet_spell_t::impact( s );

      td( s -> target ) -> water_jet
                        -> trigger(1, buff_t::DEFAULT_VALUE(), 1.0,
                                   dot_duration *
                                   p() -> composite_spell_haste());

      // Trigger hidden proxy water jet for the mage, so
      // debuff.water_jet.<expression> works
      p() -> o() -> get_target_data( s -> target ) -> debuffs.water_jet
          -> trigger(1, buff_t::DEFAULT_VALUE(), 1.0,
                     dot_duration * p() -> composite_spell_haste());
    }

    virtual double action_multiplier() const override
    {
      double am = mage_pet_spell_t::action_multiplier();

      if ( p() -> o() -> spec.icicles -> ok() )
      {
        am *= 1.0 + p() -> o() -> cache.mastery_value();
      }

      return am;
    }

    virtual void last_tick( dot_t* d ) override
    {
      mage_pet_spell_t::last_tick( d );
      td( d -> target ) -> water_jet -> expire();
    }

    bool ready() override
    {
      if ( ! p() -> o() -> perks.improved_water_ele -> ok() )
        return false;

      // Not ready, until the owner gives permission to cast
      if ( ! autocast && ! queued )
        return false;

      return mage_pet_spell_t::ready();
    }

    void reset() override
    {
      mage_pet_spell_t::reset();

      queued = false;
    }
  };

  water_elemental_pet_td_t* td( player_t* t ) const
  { return get_target_data( t ); }

  target_specific_t<water_elemental_pet_td_t> target_data;

  virtual water_elemental_pet_td_t* get_target_data( player_t* target ) const override
  {
    water_elemental_pet_td_t*& td = target_data[ target ];
    if ( ! td )
      td = new water_elemental_pet_td_t( target, const_cast<water_elemental_pet_t*>(this) );
    return td;
  }

  struct waterbolt_t: public mage_pet_spell_t
  {
    waterbolt_t( water_elemental_pet_t* p, const std::string& options_str ):
      mage_pet_spell_t( "waterbolt", p, p -> find_pet_spell( "Waterbolt" ) )
    {
      trigger_gcd = timespan_t::zero();
      parse_options( options_str );
      may_crit = true;
    }

    const water_elemental_pet_t* p() const
    { return static_cast<water_elemental_pet_t*>( player ); }

    virtual double action_multiplier() const override
    {
      double am = mage_pet_spell_t::action_multiplier();

      if ( p() -> o() -> spec.icicles -> ok() )
      {
        am *= 1.0 + p() -> o() -> cache.mastery_value();
      }

      return am;
    }
  };

  water_elemental_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "water_elemental" )
  {
    owner_coeff.sp_from_sp = 0.75;

    action_list_str  = "water_jet/waterbolt";
  }

  mage_t* o()
  { return static_cast<mage_t*>( owner ); }
  const mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "freeze"     ) return new     freeze_t( this, options_str );
    if ( name == "waterbolt"  ) return new  waterbolt_t( this, options_str );
    if ( name == "water_jet"  ) return new  water_jet_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o() -> buffs.rune_of_power -> check() )
    {
      m *= 1.0 + o() -> buffs.rune_of_power -> data().effectN( 3 ).percent();
    }

    if ( o() -> talents.incanters_flow -> ok() )
    {
      m *= 1.0 + o() -> buffs.incanters_flow -> current_stack *
                 o() -> incanters_flow_stack_mult;
    }

    m *= o() -> pet_multiplier;

    return m;
  }
};

water_elemental_pet_td_t::water_elemental_pet_td_t( player_t* target, water_elemental_pet_t* welly ) :
  actor_target_data_t( target, welly )
{
  water_jet = buff_creator_t( *this, "water_jet", welly -> find_spell( 135029 ) ).cd( timespan_t::zero() );
}

// ==========================================================================
// Pet Mirror Image
// ==========================================================================

struct mirror_image_pet_t : public pet_t
{
  struct mirror_image_spell_t : public mage_pet_spell_t
  {
    mirror_image_spell_t( const std::string& n, mirror_image_pet_t* p, const spell_data_t* s ):
      mage_pet_spell_t( n, p, s )
    {
      may_crit = true;
    }

    bool init_finished() override
    {
      if ( p() -> o() -> pets.mirror_images[ 0 ] )
      {
        stats = p() -> o() -> pets.mirror_images[ 0 ] -> get_stats( name_str );
      }

      return mage_pet_spell_t::init_finished();
    }

    mirror_image_pet_t* p() const
    { return static_cast<mirror_image_pet_t*>( player ); }
  };

  struct arcane_blast_t : public mirror_image_spell_t
  {
    arcane_blast_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "arcane_blast", p, p -> find_pet_spell( "Arcane Blast" ) )
    {
      parse_options( options_str );
    }

    virtual void execute() override
    {
      mirror_image_spell_t::execute();

      p() -> arcane_charge -> trigger();
    }

    virtual double action_multiplier() const override
    {
      double am = mirror_image_spell_t::action_multiplier();

      am *= 1.0 + p() -> arcane_charge -> stack() *
                  p() -> o() -> spec.arcane_charge -> effectN( 4 ).percent() *
                  ( 1.0 + p() -> o() -> sets.set( SET_CASTER, T15, B4 )
                                     -> effectN( 1 ).percent() );

      return am;
    }
  };

  struct fireball_t : public mirror_image_spell_t
  {
    fireball_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "fireball", p, p -> find_pet_spell( "Fireball" ) )
    {
      parse_options( options_str );
    }
  };

  struct frostbolt_t : public mirror_image_spell_t
  {
    frostbolt_t( mirror_image_pet_t* p, const std::string& options_str ):
      mirror_image_spell_t( "frostbolt", p, p -> find_pet_spell( "Frostbolt" ) )
    {
      parse_options( options_str );
    }
  };

  buff_t* arcane_charge;

  mirror_image_pet_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "mirror_image", true ),
    arcane_charge( nullptr )
  {
    owner_coeff.sp_from_sp = 1.00;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "arcane_blast" ) return new arcane_blast_t( this, options_str );
    if ( name == "fireball"     ) return new     fireball_t( this, options_str );
    if ( name == "frostbolt"    ) return new    frostbolt_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

  mage_t* o() const
  { return static_cast<mage_t*>( owner ); }

  virtual void init_action_list() override
  {

      if ( o() -> specialization() == MAGE_FIRE )
      {
        action_list_str = "fireball";
      }
      else if ( o() -> specialization() == MAGE_ARCANE )
      {
        action_list_str = "arcane_blast";
      }
      else
      {
        action_list_str = "frostbolt";
      }

    pet_t::init_action_list();
  }

  virtual void create_buffs() override
  {
    pet_t::create_buffs();

    arcane_charge = buff_creator_t( this, "arcane_charge",
                                    o() -> spec.arcane_charge );
  }


  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    m *= o() -> pet_multiplier;

    return m;
  }
};

// ==========================================================================
// Pet Prismatic Crystal
// ==========================================================================

struct prismatic_crystal_t : public pet_t
{
  struct prismatic_crystal_aoe_state_t : public action_state_t
  {
    action_t* owner_action;

    prismatic_crystal_aoe_state_t( action_t* action, player_t* target ) :
      action_state_t( action, target ), owner_action( nullptr )
    { }

    void initialize() override
    { action_state_t::initialize(); owner_action = nullptr; }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    { action_state_t::debug_str( s ) << " owner_action=" << ( owner_action ? owner_action -> name_str : "unknown" ); return s; }

    void copy_state( const action_state_t* other ) override
    {
      action_state_t::copy_state( other );

      owner_action = debug_cast<const prismatic_crystal_aoe_state_t*>( other ) -> owner_action;
    }
  };

  struct prismatic_crystal_aoe_t : public spell_t
  {
    prismatic_crystal_aoe_t( prismatic_crystal_t* p ) :
      spell_t( "prismatic_crystal", p, p -> find_spell( 155152 ) )
    {
      school = SCHOOL_ARCANE;
      may_crit = may_miss = callbacks = false;
      background = true;
      aoe = -1;
      split_aoe_damage = true;
    }

    void init() override
    {
      spell_t::init();

      // Don't snapshot anything
      snapshot_flags = 0;
    }

    prismatic_crystal_t* p()
    { return debug_cast<prismatic_crystal_t*>( player ); }

    void execute() override
    {
      // Note, the pre_execute_state is guaranteed to be set, as
      // prismatic_crystal_t::assess_damage() will always schedule the aoe
      // execute with a state
      const prismatic_crystal_aoe_state_t* st = debug_cast<const prismatic_crystal_aoe_state_t*>( pre_execute_state );

      stats = p() -> add_proxy_stats( st -> owner_action );

      spell_t::execute();
    }

    action_state_t* new_state() override
    { return new prismatic_crystal_aoe_state_t( this, target ); }
  };

  prismatic_crystal_aoe_t* aoe_spell;
  const spell_data_t* damage_taken,
                    * frost_damage_taken;
  std::map<size_t, std::vector<stats_t*> > proxy_stats;

  prismatic_crystal_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "prismatic_crystal", true ),
    aoe_spell( nullptr ),
    damage_taken( owner -> find_spell( 155153 ) ),
    frost_damage_taken( owner -> find_spell( 152087 ) )
  { true_level = 101; }

  stats_t* add_proxy_stats( action_t* owner_action )
  {
    std::vector<stats_t*>& stats_data = proxy_stats[ owner_action -> player -> actor_index ];

    if ( stats_data.size() <= owner_action -> internal_id )
      stats_data.resize( owner_action -> internal_id + 1, nullptr );

    if ( stats_data[ owner_action -> internal_id ] == nullptr )
    {
      if ( owner_action -> player -> is_pet() )
      {
        stats_data[ owner_action -> internal_id ] = get_stats( owner_action -> name_str + "_" + owner_action -> player -> name_str );
      }
      else
      {
        stats_data[ owner_action -> internal_id ] = get_stats( owner_action -> name_str );
      }

      stats_data[ owner_action -> internal_id ] -> action_list.push_back( owner_action );
      stats_data[ owner_action -> internal_id ] -> school = owner_action -> school;
    }

    return stats_data[ owner_action -> internal_id ];
  }

  mage_t* o() const
  { return debug_cast<mage_t*>( owner ); }

  void init_spells() override
  {
    pet_t::init_spells();

    aoe_spell = new prismatic_crystal_aoe_t( this );
  }

  void arise() override
  {
    pet_t::arise();

    // For now, when Prismatic Crystal is summoned, adjust all mage targets to it.
    o() -> current_target = this;
    for ( size_t i = 0, end = o() -> action_list.size(); i < end; i++ )
      o() -> action_list[i] -> target_cache.is_valid = false;
  }

  void demise() override
  {
    pet_t::demise();

    // For now, when Prismatic Crystal despawns, adjust all mage targets back
    // to fluffy pillow and invalid all mage and pet action target caches
    o() -> current_target = o() -> target;
    for ( size_t i = 0, end = o() -> action_list.size(); i < end; i++ )
    {
      o() -> action_list[i] -> target = o() -> current_target;
      o() -> action_list[i] -> target_cache.is_valid = false;
    }

    for (auto pet : o() -> pet_list)
    {
      

      pet -> target = o() -> target;
      for ( size_t j = 0, j_end = pet -> action_list.size(); j < j_end; j++ )
      {
        pet -> action_list[j] -> target = pet -> target;
        pet -> action_list[j] -> target_cache.is_valid = false;
      }
    }
  }

  double composite_mitigation_versatility() const override { return 0; }

  double composite_player_vulnerability( school_e school ) const override
  {
    double m = pet_t::composite_player_vulnerability( school );

    if ( o() -> specialization() == MAGE_FROST )
    {
      m *= 1.0 + frost_damage_taken -> effectN( 3 ).percent();
    }
    else
    {
      m *= 1.0 + damage_taken -> effectN( 1 ).percent();
    }

    return m;
  }

  void assess_damage( school_e school, dmg_e type, action_state_t* state ) override
  {
    base_t::assess_damage( school, type, state );

    if ( state -> result_amount == 0 )
      return;

    // Get explicit state object so we can manually add the correct source
    // action to the PC aoe, so reporting looks nice
    action_state_t* new_state = aoe_spell -> get_state();
    prismatic_crystal_aoe_state_t* st = debug_cast<prismatic_crystal_aoe_state_t*>( new_state );
    st -> owner_action = state -> action;

    // Reset target to an enemy to avoid infinite looping
    new_state -> target = aoe_spell -> target;
    new_state -> result_type = DMG_DIRECT;

    // Set PC Aoe damage through base damage, so we can get correct split
    // shenanigans going on in calculate_direct_amount()
    aoe_spell -> base_dd_min = state -> result_total;
    aoe_spell -> base_dd_max = state -> result_total;
    aoe_spell -> pre_execute_state = new_state;
    aoe_spell -> execute();
  }
};

// ==========================================================================
// Pet Temporal Heroes (2T18)
// ==========================================================================

struct temporal_hero_t : public pet_t
{
  hero_e hero_type;

  struct temporal_hero_melee_attack_t : public mage_pet_melee_attack_t
  {
    temporal_hero_melee_attack_t( pet_t* p ) :
      mage_pet_melee_attack_t( "melee", p )
    {
      may_crit = true;
      background = repeating = auto_attack = true;
      school = SCHOOL_PHYSICAL;
      special = false;
      weapon = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
    }

    bool init_finished() override
    {
      pet_t* p = debug_cast<pet_t*>( player );
      mage_t* m = debug_cast<mage_t*>( p -> owner );

      if ( m -> pets.temporal_heroes[0] )
      {
        stats = m -> pets.temporal_heroes[0] -> get_stats( "melee" );
      }

      return mage_pet_melee_attack_t::init_finished();
    }
  };

  struct temporal_hero_autoattack_t : public mage_pet_melee_attack_t
  {
    temporal_hero_autoattack_t( pet_t* p ) :
      mage_pet_melee_attack_t( "auto_attack", p )
    {
      p -> main_hand_attack = new temporal_hero_melee_attack_t( p );
      trigger_gcd = timespan_t::zero();
    }

    virtual void execute() override
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready() override
    {
      temporal_hero_t* hero = debug_cast<temporal_hero_t*>( player );
      if ( hero -> hero_type != ARTHAS )
      {
        return false;
      }

      return player -> main_hand_attack -> execute_event == nullptr;
    }
  };

  struct temporal_hero_frostbolt_t : public mage_pet_spell_t
  {
    temporal_hero_frostbolt_t( pet_t* p ) :
      mage_pet_spell_t( "frostbolt", p, p -> find_spell( 191764 ) )
    {
      base_dd_min = base_dd_max = 2750.0;
      may_crit = true;
    }

    virtual bool ready() override
    {
      temporal_hero_t* hero = debug_cast<temporal_hero_t*>( player );
      if ( hero -> hero_type != JAINA )
      {
        return false;
      }

      return mage_pet_spell_t::ready();
    }

    bool init_finished() override
    {
      pet_t* p = debug_cast<pet_t*>( player );
      mage_t* m = debug_cast<mage_t*>( p -> owner );

      if ( m -> pets.temporal_heroes[0] )
      {
        stats = m -> pets.temporal_heroes[0] -> get_stats( "frostbolt" );
      }

      return mage_pet_spell_t::init_finished();
    }
  };

  struct temporal_hero_shoot_t : public mage_pet_spell_t
  {
    temporal_hero_shoot_t( pet_t* p ) :
      mage_pet_spell_t( "shoot", p, p -> find_spell( 191799 ) )
    {
      school = SCHOOL_PHYSICAL;
      base_dd_min = base_dd_max = 3255.19;
      base_execute_time = p -> main_hand_weapon.swing_time;
      may_crit = true;
    }

    virtual bool ready() override
    {
      temporal_hero_t* hero = debug_cast<temporal_hero_t*>( player );
      if ( hero -> hero_type != SYLVANAS && hero -> hero_type != TYRANDE )
      {
        return false;
      }

      return player -> main_hand_attack -> execute_event == nullptr;
    }

    bool init_finished() override
    {
      pet_t* p = debug_cast<pet_t*>( player );
      mage_t* m = debug_cast<mage_t*>( p -> owner );

      if ( m -> pets.temporal_heroes[0] )
      {
        stats = m -> pets.temporal_heroes[0] -> get_stats( "shoot" );
      }

      return mage_pet_spell_t::init_finished();
    }
  };

  // Each Temporal Hero has base damage and hidden multipliers for damage done
  // Values are reverse engineered from 6.2 PTR Build 20157 testing
  double temporal_hero_multiplier;

  temporal_hero_t( sim_t* sim, mage_t* owner ) :
    pet_t( sim, owner, "temporal_hero", true, true ),
    hero_type( ARTHAS ), temporal_hero_multiplier( 1.0 )
  { }

  void init_base_stats() override
  {
    owner_coeff.ap_from_sp = 11.408;
    owner_coeff.sp_from_sp = 5.0;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 7262.57;
    main_hand_weapon.max_dmg    = 7262.57;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    pet_t::init_base_stats();
  }

  void init_action_list() override
  {
    action_list_str = "auto_attack/frostbolt/shoot";

    use_default_action_list = true;

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "auto_attack" ) return new temporal_hero_autoattack_t( this );
    if ( name == "frostbolt" )   return new  temporal_hero_frostbolt_t( this );
    if ( name == "shoot" )       return new      temporal_hero_shoot_t( this );

    return base_t::create_action( name, options_str );
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    // Temporal Hero benefits from Temporal Power applied by itself (1 stack).
    // Using owner's buff object, in order to avoid creating a separate buff_t
    // for each pet instance, and merging the buff statistics.
    if ( owner -> sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
    {
      mage_t* mage = debug_cast<mage_t*>( owner );
      m *= 1.0 + mage -> buffs.temporal_power -> data().effectN( 1 ).percent();
    }

    m *= temporal_hero_multiplier;

    return m;
  }

  void arise() override
  {
    pet_t::arise();
    mage_t* m = debug_cast<mage_t*>( owner );

    // Summoned heroes follow Jaina -> Arthas -> Sylvanas -> Tyrande order
    if ( m -> last_summoned == JAINA )
    {
      hero_type = ARTHAS;
      temporal_hero_multiplier = 0.1964;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s summons 2T18 temporal hero: Arthas",
                                 owner -> name() );
      }
    }
    else if ( m -> last_summoned == TYRANDE )
    {
      hero_type = JAINA;
      temporal_hero_multiplier = 0.6;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s summons 2T18 temporal hero: Jaina" ,
                                 owner -> name() );
      }
    }
    else if ( m -> last_summoned == ARTHAS )
    {
      hero_type = SYLVANAS;
      temporal_hero_multiplier = 0.5283;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s summons 2T18 temporal hero: Sylvanas",
                                 owner -> name() );
      }
    }
    else
    {
      hero_type = TYRANDE;
      temporal_hero_multiplier = 0.5283;

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s summons 2T18 temporal hero: Tyrande",
                                 owner -> name() );
      }
    }

    m -> last_summoned = hero_type;

    if ( owner -> sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
    {
      mage_t* m = debug_cast<mage_t*>( owner );
      m -> buffs.temporal_power -> trigger();

      owner -> invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    }
  }

  void demise() override
  {
    pet_t::demise();

    mage_t* m = debug_cast<mage_t*>( owner );
    m -> buffs.temporal_power -> decrement();

    owner -> invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  static void randomize_last_summoned( mage_t* m )
  {
    double rand = m -> rng().real();
    if ( rand < 0.25 )
    {
      m -> last_summoned = ARTHAS;
    }
    else if ( rand < 0.5 )
    {
      m -> last_summoned = JAINA;
    }
    else if ( rand < 0.75 )
    {
      m -> last_summoned = SYLVANAS;
    }
    else
    {
      m -> last_summoned = TYRANDE;
    }
  }
};

} // pets

namespace actions {
// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_t : public spell_t
{
  bool frozen, may_proc_missiles, consumes_ice_floes;
  bool hasted_by_pom; // True if the spells time_to_execute was set to zero exactly because of Presence of Mind

private:
  // Helper variable to disable the functionality of PoM in mage_spell_t::execute_time(),
  // to check if the spell would be instant or not without PoM.
  bool pom_enabled;

public:
  mage_spell_t( const std::string& n, mage_t* p,
                const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    frozen( false ),
    may_proc_missiles( true ),
    consumes_ice_floes( true ),
    hasted_by_pom( false ),
    pom_enabled( true )
  {
    may_crit      = true;
    tick_may_crit = true;
  }

  mage_t* p()
  { return static_cast<mage_t*>( player ); }
  const mage_t* p() const
  { return static_cast<mage_t*>( player ); }

  mage_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double cost() const override
  {
    double c = spell_t::cost();

    if ( p() -> buffs.arcane_power -> check() )
    {
      double m = 1.0 + p() -> buffs.arcane_power -> data().effectN( 2 ).percent() + p() -> perks.improved_arcane_power -> effectN( 1 ).percent();

      c *= m;
    }

    return c;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = spell_t::execute_time();

    if ( ! channeled && pom_enabled && t > timespan_t::zero() && p() -> buffs.presence_of_mind -> up() )
      return timespan_t::zero();
    return t;
  }

  virtual double action_multiplier() const override
  {
    double am=spell_t::action_multiplier();
    if ( p() -> specialization() == MAGE_ARCANE )
    {
      double mana_pct= p() -> resources.pct( RESOURCE_MANA );
      am *= 1.0 + mana_pct * p() -> composite_mastery_value();
    }
    return am;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    // If there is no state to schedule, make one and put the actor's current
    // target into it. This guarantees that:
    // 1) action_execute_event_t::execute() does not execute on dead targets, if the target died during cast
    // 2) We do not modify action's variables this early in the game
    //
    // If this is a tick action, there's going to be a state object passed to
    // it, that will have the correct target anyhow, since the parent will have
    // had it's target adjusted during its execution.
    if ( state == nullptr )
    {
      state = get_state();

      // If cycle_targets or target_if option is used, we need to target the spell to the (found)
      // target of the action, as it was selected during the action_t::ready() call.
      if ( cycle_targets == 1 || target_if_mode != TARGET_IF_NONE )
        state -> target = target;
      // Put the actor's current target into the state object always.
      else
        state -> target = p() -> current_target;
    }

    spell_t::schedule_execute( state );

    if ( ! channeled )
    {
      assert( pom_enabled );
      pom_enabled = false;
      if ( execute_time() > timespan_t::zero() && p() -> buffs.presence_of_mind -> check() )
      {
        hasted_by_pom = true;
      }
      pom_enabled = true;
    }
  }

  virtual bool usable_moving() const override
  {
    if ( p() -> buffs.ice_floes -> check() )
      return true;

    return spell_t::usable_moving();
  }
  virtual double composite_crit_multiplier() const override
  {
    double m = spell_t::composite_crit_multiplier();
    if ( frozen && p() -> passives.shatter -> ok() )
      m *= 1.5;
    return m;
  }

  void snapshot_internal( action_state_t* s, uint32_t flags, dmg_e rt ) override
  {
    spell_t::snapshot_internal( s, flags, rt );
    // Shatter's +50% crit bonus needs to be added after multipliers etc
    if ( ( flags & STATE_CRIT ) && frozen && p() -> passives.shatter -> ok() )
      s -> crit += p() -> passives.shatter -> effectN( 2 ).percent();
  }

  virtual void expire_heating_up() // delay 0.25s the removal of heating up on non-critting spell with travel time or scorch
  {
    mage_t* p = this -> p();

    if ( ! travel_speed )
    {
      p -> buffs.heating_up -> expire();
    }
    else
    {
      // delay heating up removal
      if ( sim -> log ) sim -> out_log << "Heating up delay by 0.25s";
      p -> buffs.heating_up -> expire( timespan_t::from_millis( 250 ) );
    }
  }

  void trigger_hot_streak( action_state_t* s )
  {
    mage_t* p = this -> p();

    p -> procs.test_for_crit_hotstreak -> occur();

    if ( s -> result == RESULT_CRIT )
    {
      p -> procs.crit_for_hotstreak -> occur();

      if ( ! p -> buffs.heating_up -> up() )
      {
        p -> buffs.heating_up -> trigger();
      }
      else
      {
        p -> procs.hotstreak  -> occur();
        p -> buffs.heating_up -> expire();
        p -> buffs.pyroblast  -> trigger();
        if ( p -> sets.has_set_bonus( MAGE_FIRE, T17, B4 ) && p -> rppm_pyromaniac.trigger() )
          p -> buffs.pyromaniac -> trigger();
      }
    }
    else
    {
      if ( p -> buffs.heating_up -> up() ) expire_heating_up();
    }
  }

  void trigger_icicle_gain( action_state_t* state, stats_t* stats )
  {
    if ( ! p() -> spec.icicles -> ok() )
      return;

    if ( ! result_is_hit_or_multistrike( state -> result ) )
      return;

    // Icicles do not double dip on target based multipliers
    double amount = state -> result_amount / ( state -> target_da_multiplier * state -> versatility ) * p() -> cache.mastery_value();

    assert( as<int>( p() -> icicles.size() ) <= p() -> spec.icicles -> effectN( 2 ).base_value() );
    // Shoot one
    if ( as<int>( p() -> icicles.size() ) == p() -> spec.icicles -> effectN( 2 ).base_value() )
      p() -> trigger_icicle( state );
    p() -> icicles.push_back( icicle_tuple_t( p() -> sim -> current_time(), icicle_data_t( amount, stats ) ) );

    if ( p() -> sim -> debug )
      p() -> sim -> out_debug.printf( "%s icicle gain, damage=%f, total=%u",
                                      p() -> name(),
                                      amount,
                                      as<unsigned>(p() -> icicles.size() ) );
  }

  virtual void execute() override
  {
    player_t* original_target = nullptr;
    // Mage spells will always have a pre_execute_state defined, because of
    // schedule_execute() trickery.
    //
    // Adjust the target of this action to always match what the
    // pre_execute_state targets. Note that execute() will never be called if
    // the actor's current target (at the time of cast beginning) has demised
    // before the cast finishes.
    if ( pre_execute_state )
    {
      // Adjust target if necessary
      if ( target != pre_execute_state -> target )
      {
        original_target = target;
        target = pre_execute_state -> target;
      }

      // Massive hack to describe a situation where schedule_execute()
      // forcefully made a pre-execute state to pass the current target to
      // execute. In this case we release the pre_execute_state, because we
      // want the action to snapshot it's own stats on "cast finish". We have,
      // however changed the target of the action to the one specified whe nthe
      // casting begun (in schedule_execute()).
      if ( pre_execute_state -> result_type == RESULT_TYPE_NONE )
      {
        action_state_t::release( pre_execute_state );
        pre_execute_state = nullptr;
      }
    }

    // Regenerate just before executing the spell, so Arcane mages have a 100%
    // correct mana level to snapshot their multiplier with
    if ( p() -> specialization() == MAGE_ARCANE &&
         p() -> regen_type == REGEN_DYNAMIC )
      p() -> do_dynamic_regen();

    spell_t::execute();

    // Restore original target if necessary
    if ( original_target )
      target = original_target;

    if ( background )
      return;

    if ( ! channeled && hasted_by_pom )
    {
      p() -> buffs.presence_of_mind -> expire();
      hasted_by_pom = false;
    }

    if ( execute_time() > timespan_t::zero() && consumes_ice_floes && p() -> buffs.ice_floes -> up() )
    {
      p() -> buffs.ice_floes -> decrement();
    }

    if ( !harmful || background )
    {
      may_proc_missiles = false;
    }

    if ( p() -> specialization() == MAGE_ARCANE &&
         result_is_hit( execute_state -> result ) &&
         may_proc_missiles )
    {
      p() -> buffs.arcane_missiles -> trigger();
    }
  }

  virtual void reset() override
  {
    spell_t::reset();

    hasted_by_pom = false;
  }

  void trigger_ignite( action_state_t* state );
  void trigger_unstable_magic( action_state_t* state );

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    spell_t::available_targets( tl );

    if ( p() -> pets.prismatic_crystal &&
         target != p() -> pets.prismatic_crystal &&
         ! p() -> pets.prismatic_crystal -> is_sleeping() )
      tl.push_back( p() -> pets.prismatic_crystal );

    return tl.size();
  }

  void record_data( action_state_t* data ) override
  {
    if ( data -> target == p() -> pets.prismatic_crystal )
      return;

    spell_t::record_data( data );
  }

  bool init_finished() override
  {
    pets::prismatic_crystal_t* pc = debug_cast<pets::prismatic_crystal_t*>( p() -> pets.prismatic_crystal );
    if ( pc )
    {
      pc -> add_proxy_stats( this );
    }

    return spell_t::init_finished();
  }
};

typedef residual_action::residual_periodic_action_t< mage_spell_t > residual_action_t;

// Icicles ==================================================================

struct icicle_state_t : public action_state_t
{
  stats_t* source;

  icicle_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), source( nullptr )
  { }

  void initialize() override
  { action_state_t::initialize(); source = nullptr; }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  { action_state_t::debug_str( s ) << " source=" << ( source ? source -> name_str : "unknown" ); return s; }

  void copy_state( const action_state_t* other ) override
  {
    action_state_t::copy_state( other );

    source = debug_cast<const icicle_state_t*>( other ) -> source;
  }
};

struct icicle_t : public mage_spell_t
{
  icicle_t( mage_t* p ) :
    mage_spell_t( "icicle", p, p -> find_spell( 148022 ) )
  {
    may_crit = false;
    may_multistrike = 0;
    proc = background = true;

    if ( p -> glyphs.splitting_ice -> ok() )
    {
      aoe = 1 + p -> glyphs.splitting_ice -> effectN( 1 ).base_value();
      base_aoe_multiplier = p -> glyphs.splitting_ice
                              -> effectN( 2 ).percent();
    }
  }

  // To correctly record damage and execute information to the correct source
  // action (FB or FFB), we set the stats object of the icicle cast to the
  // source stats object, carried from trigger_icicle() to here through the
  // execute_event_t.
  void execute() override
  {
    const icicle_state_t* is = debug_cast<const icicle_state_t*>( pre_execute_state );
    assert( is -> source );
    stats = is -> source;

    mage_spell_t::execute();
  }

  // Due to the mage targeting system, the pre_execute_state in is emptied by
  // the mage_spell_t::execute() call (before getting to action_t::execute()),
  // thus we need to "re-set" the stats object into the state object that is
  // used for the next leg of the execution path (execute() using travel event
  // to impact()). This is done in schedule_travel().
  void schedule_travel( action_state_t* state ) override
  {
    icicle_state_t* is = debug_cast<icicle_state_t*>( state );
    is -> source = stats;

    mage_spell_t::schedule_travel( state );
  }

  // And again, once the icicle impacts, we set the stats object here again
  // because multiple icicles can be executing, causing the state object to be
  // set to another object between the execution of this specific icicle, and
  // the impact.
  void impact( action_state_t* state ) override
  {
    // Splitting Ice does not cleave onto Prismatic Crystal
    if ( state -> target == p() -> pets.prismatic_crystal &&
         state -> chain_target > 0 )
    {
      return;
    }

    const icicle_state_t* is = debug_cast<const icicle_state_t*>( state );
    assert( is -> source );
    stats = is -> source;

    mage_spell_t::impact( state );
  }

  action_state_t* new_state() override
  { return new icicle_state_t( this, target ); }

  void init() override
  {
    mage_spell_t::init();

    snapshot_flags &= ~( STATE_MUL_DA | STATE_SP | STATE_CRIT | STATE_TGT_CRIT );
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public mage_spell_t
{
  presence_of_mind_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "presence_of_mind", p, p -> find_class_spell( "Presence of Mind" )  )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual bool ready() override
  {
    if ( p() -> buffs.presence_of_mind -> check() )
    {
      return false;
    }

    return mage_spell_t::ready();
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    p() -> buffs.presence_of_mind -> trigger();
  }
};

// Ignite ===================================================================

struct ignite_t : public residual_action_t
{
  ignite_t( mage_t* player ) :
    residual_action_t( "ignite", player, player -> find_spell( 12846 ) )
  {
    dot_duration = dbc::find_spell( player, 12654 ) -> duration();
    base_tick_time = dbc::find_spell( player, 12654 ) -> effectN( 1 ).period();
    school = SCHOOL_FIRE;
  }
};

void mage_spell_t::trigger_ignite( action_state_t* state )
{
  mage_t& p = *this -> p();
  if ( ! p.active_ignite ) return;
  double amount = state -> result_amount * p.cache.mastery_value();
  trigger( p.active_ignite, state -> target, amount );
}

// Arcane Barrage Spell =====================================================

struct arcane_barrage_t : public mage_spell_t
{
  arcane_barrage_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_barrage", p, p -> find_class_spell( "Arcane Barrage" ) )
  {
    parse_options( options_str );

    base_aoe_multiplier *= data().effectN( 2 ).percent();
  }

  virtual void execute() override
  {
    int charges = p() -> buffs.arcane_charge -> check();
    aoe = charges <= 0 ? 0 : 1 + charges;

    for ( int i = 0; i < ( int ) sizeof_array( p() -> benefits.arcane_charge ); i++ )
    {
      p() -> benefits.arcane_charge[ i ] -> update( i == charges );
    }

    mage_spell_t::execute();

    p() -> buffs.arcane_charge -> expire();
  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() *
                p() -> spec.arcane_charge -> effectN( 1 ).percent() *
                ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4 )
                            -> effectN( 1 ).percent() );

    return am;
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t : public mage_spell_t
{
  double wild_arcanist_effect;

  arcane_blast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_blast", p, p -> find_class_spell( "Arcane Blast" ) ),
    wild_arcanist_effect( 0.0 )
  {
    parse_options( options_str );

    if ( p -> wild_arcanist )
    {
      const spell_data_t* data = p -> wild_arcanist -> driver();
      wild_arcanist_effect = std::fabs( data -> effectN( 1 ).average( p -> wild_arcanist -> item ) );
      wild_arcanist_effect /= 100.0;
    }
  }

  virtual double cost() const override
  {
    double c = mage_spell_t::cost();

    if ( p() -> buffs.arcane_charge -> check() )
    {
      c *= 1.0 +  p() -> buffs.arcane_charge -> check() *
                  p() -> spec.arcane_charge -> effectN( 2 ).percent() *
                  ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4 )
                              -> effectN( 1 ).percent() );
    }

    if ( p() -> buffs.profound_magic -> check() )
    {
      c *= 1.0 - p() -> buffs.profound_magic -> stack() * 0.25;
    }

    if ( p() -> buffs.arcane_affinity -> check() )
    {
      c *= 1.0 + p() -> buffs.arcane_affinity -> data().effectN( 1 ).percent();
    }

    return c;
  }

  virtual void execute() override
  {
    for ( unsigned i = 0; i < sizeof_array( p() -> benefits.arcane_charge ); i++ )
    {
      p() -> benefits.arcane_charge[ i ] -> update( as<int>( i ) == p() -> buffs.arcane_charge -> check() );
    }

    mage_spell_t::execute();

    p() -> buffs.arcane_charge -> trigger();
    p() -> buffs.profound_magic -> expire();

    if ( p() -> sets.has_set_bonus( MAGE_ARCANE, T17, B4 ) &&
         p() -> rppm_arcane_instability.trigger() )
    {
      p() -> buffs.arcane_instability -> trigger();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() *
                p() -> spec.arcane_charge -> effectN( 1 ).percent() *
                ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4 )
                            -> effectN( 1 ).percent() );

    if ( p() -> wild_arcanist && p() -> buffs.arcane_power -> check() )
    {
      am *= 1.0 + wild_arcanist_effect;
    }

    return am;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = mage_spell_t::execute_time();

    if ( p() -> perks.enhanced_arcane_blast -> ok() )
    {
      t *=  1.0 - p() -> buffs.arcane_charge -> stack() *
                  p() -> perks.enhanced_arcane_blast -> effectN( 1 ).percent();
    }

    if ( p() -> buffs.arcane_affinity -> up() )
    {
      t *= 1.0 + p() -> buffs.arcane_affinity -> data().effectN( 1 ).percent();
    }

    if ( p() -> wild_arcanist && p() -> buffs.arcane_power -> check() )
    {
      t *= 1.0 - wild_arcanist_effect;
    }

    return t;
  }

  virtual timespan_t gcd() const override
  {
    timespan_t t = mage_spell_t::gcd();

    if ( p() -> perks.enhanced_arcane_blast -> ok() )
    {
      t *= ( 1 - p() -> buffs.arcane_charge -> stack() *
                 p() -> perks.enhanced_arcane_blast -> effectN( 1 ).percent());
      t = std::max( timespan_t::from_seconds( 1.0 ), t );
    }

    if ( p() -> wild_arcanist && p() -> buffs.arcane_power -> check() )
    {
      // Hidden GCD cap on ToSW
      t = std::max( timespan_t::from_seconds( 1.2 ), t );
      t *= 1.0 - wild_arcanist_effect;
    }

    return t;
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit_or_multistrike( s -> result ) )
    {
      if ( p() -> talents.unstable_magic -> ok() )
      {
        trigger_unstable_magic( s );
      }
    }
  }

};

// Arcane Brilliance Spell ==================================================

struct arcane_brilliance_t : public mage_spell_t
{
  arcane_brilliance_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_brilliance", p, p -> find_class_spell( "Arcane Brilliance" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    harmful = false;
    background = ( sim -> overrides.spell_power_multiplier != 0 && sim -> overrides.critical_strike != 0 );
  }

  virtual void execute() override
  {
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    if ( ! sim -> overrides.spell_power_multiplier )
      sim -> auras.spell_power_multiplier -> trigger();

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();
  }
};

// Arcane Explosion Spell ===========================================================

struct arcane_explosion_t : public mage_spell_t
{
  arcane_explosion_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_explosion", p, p -> find_class_spell( "Arcane Explosion" ) )
  {
    parse_options( options_str );
    aoe = -1;

    if ( p -> glyphs.arcane_explosion -> ok() )
    {
      radius += p -> glyphs.arcane_explosion -> effectN( 2 ).base_value();
    }
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( rng().roll ( data().effectN( 2 ).percent() ) )
      {
        p() -> buffs.arcane_charge -> trigger();

        if ( p() -> sets.has_set_bonus( MAGE_ARCANE, T17, B4 ) &&
             p() -> rppm_arcane_instability.trigger() )
        {
          p() -> buffs.arcane_instability -> trigger();
        }
      }
      else
      {
        p() -> buffs.arcane_charge -> refresh();
      }
    }
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_tick_t : public mage_spell_t
{
  arcane_missiles_tick_t( mage_t* p ) :
    mage_spell_t( "arcane_missiles_tick", p, p -> find_class_spell( "Arcane Missiles" ) -> effectN( 2 ).trigger() )
  {
    background  = true;
    dot_duration = timespan_t::zero();
  }
};

struct arcane_missiles_t : public mage_spell_t
{
  timespan_t missiles_tick_time;
  timespan_t temporal_hero_duration;

  arcane_missiles_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_missiles", p,
                  p -> find_class_spell( "Arcane Missiles" ) ),
    missiles_tick_time( timespan_t::zero() ),
    temporal_hero_duration( timespan_t::zero() )
  {
    parse_options( options_str );
    may_miss = false;
    may_proc_missiles = false;
    dot_duration      = data().duration();
    base_tick_time    = data().effectN( 2).period();
    channeled         = true;
    hasted_ticks      = false;
    dynamic_tick_action = true;
    tick_action = new arcane_missiles_tick_t( p );
    may_miss = false;

    missiles_tick_time = base_tick_time;
    temporal_hero_duration = p -> find_spell( 188117 ) -> duration();
  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.arcane_charge -> stack() *
                p() -> spec.arcane_charge -> effectN( 1 ).percent() *
                ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4 )
                            -> effectN( 1 ).percent() );
    if ( p() -> sets.has_set_bonus( SET_CASTER, T14, B2 ) )
    {
      am *= 1.07;
    }

    return am;
  }

  // Flag Arcane Missiles as direct damage for triggering effects
  dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_DIRECT;
  }


  virtual void execute() override
  {
    for ( unsigned i = 0; i < sizeof_array( p() -> benefits.arcane_charge ); i++ )
    {
      p() -> benefits.arcane_charge[ i ] -> update( as<int>( i ) == p() -> buffs.arcane_charge -> check() );
    }

    // 4T17 : Increase the number of missiles by reducing base_tick_time
    base_tick_time = missiles_tick_time;
    if ( p() -> buffs.arcane_instability -> up() )
    {
      base_tick_time *= 1 + p() -> buffs.arcane_instability
                                -> data().effectN( 1 ).percent() ;
      p() -> buffs.arcane_instability -> expire();
    }

    mage_spell_t::execute();

    if ( p() -> buffs.arcane_power -> check() && p() -> talents.overpowered -> ok() )
      p() -> buffs.arcane_power -> extend_duration( p(), timespan_t::from_seconds( p() -> talents.overpowered -> effectN( 1 ).base_value() ) );

    p() -> buffs.arcane_missiles -> up();

    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
    {
      p() -> buffs.profound_magic -> trigger();
    }

    // T16 4pc has a chance not to consume arcane missiles buff
    if ( !p() -> sets.has_set_bonus( SET_CASTER, T16, B4 ) || !rng().roll( p() -> sets.set( SET_CASTER, T16, B4 ) -> effectN( 1 ).percent() ) )
    {
      p() -> buffs.arcane_missiles -> decrement();
    }

    if ( p() -> sets.has_set_bonus( MAGE_ARCANE, T18, B2 ) &&
         rng().roll( p() -> sets.set( MAGE_ARCANE, T18, B2 )
                         -> proc_chance() ) )
    {
      for ( unsigned i = 0; i < sizeof_array( p() -> pets.temporal_heroes ); i++ )
      {
        if ( p() -> pets.temporal_heroes[ i ] -> is_sleeping() )
        {
          p() -> pets.temporal_heroes[ i ] -> summon( temporal_hero_duration );
          break;
        }
      }
    }
  }

  virtual void last_tick ( dot_t * d) override
  {
    mage_spell_t::last_tick( d );

    p() -> buffs.arcane_charge -> trigger();

    if ( p() -> sets.has_set_bonus( MAGE_ARCANE, T17, B4 ) &&
         p() -> rppm_arcane_instability.trigger() )
    {
      p() -> buffs.arcane_instability -> trigger();
    }
  }

  virtual bool ready() override
  {
    if ( ! p() -> buffs.arcane_missiles -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Arcane Orb Spell =========================================================
struct arcane_orb_bolt_t : public mage_spell_t
{
  arcane_orb_bolt_t( mage_t* p ) :
    mage_spell_t( "arcane_orb_bolt", p, p -> find_spell( 153640 ) )
  {
    aoe = -1;
    background = true;
    dual = true;
    cooldown -> duration = timespan_t::zero(); // dbc has CD of 15 seconds
  }

  virtual void impact( action_state_t* s ) override
  {
    for ( unsigned i = 0; i < sizeof_array( p() -> benefits.arcane_charge ); i++)
    {
      p() -> benefits.arcane_charge[ i ] -> update( as<int>( i ) == p() -> buffs.arcane_charge -> check() );
    }

    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.arcane_charge -> trigger();
      p() -> buffs.arcane_missiles -> trigger();

      if ( p() -> sets.has_set_bonus( MAGE_ARCANE, T17, B4 ) &&
           p() -> rppm_arcane_instability.trigger() )
      {
        p() -> buffs.arcane_instability -> trigger();
      }
    }
  }
};

struct arcane_orb_t : public mage_spell_t
{
  arcane_orb_bolt_t* orb_bolt;

  arcane_orb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_orb", p, p -> find_talent_spell( "Arcane Orb" ) ),
    orb_bolt( new arcane_orb_bolt_t( p ) )
  {
    parse_options( options_str );

    may_miss       = false;
    may_crit       = false;
    add_child( orb_bolt );
  }

  virtual void execute() override
  {
    for ( unsigned i = 0; i < sizeof_array( p() -> benefits.arcane_charge ); i++)
    {
      p() -> benefits.arcane_charge[ i ] -> update( as<int>( i ) == p() -> buffs.arcane_charge -> check() );
    }

    mage_spell_t::execute();
    p() -> buffs.arcane_charge -> trigger();

    if ( p() -> sets.has_set_bonus( MAGE_ARCANE, T17, B4 ) &&
         p() -> rppm_arcane_instability.trigger() )
    {
      p() -> buffs.arcane_instability -> trigger();
    }
  }


  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( ( player -> current.distance - 10.0 ) /
                                     16.0 );
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    orb_bolt -> execute();
  }
};

// Arcane Power Spell =======================================================

struct arcane_power_t : public mage_spell_t
{
  arcane_power_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "arcane_power", p, p -> find_class_spell( "Arcane Power" ) )
  {
    parse_options( options_str );
    harmful = false;

    if ( p -> glyphs.arcane_power -> ok() )
    {
      cooldown -> duration *= 1.0 + p -> glyphs.arcane_power -> effectN( 2 ).percent();
    }
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    p() -> buffs.arcane_power -> trigger( 1, data().effectN( 1 ).percent() );
  }
};

// Blast Wave Spell ==========================================================

struct blast_wave_t : public mage_spell_t
{
    blast_wave_t( mage_t* p, const std::string& options_str ) :
       mage_spell_t( "blast_wave", p, p -> talents.blast_wave )
    {
        parse_options( options_str );
        base_multiplier *= 1.0 + p -> talents.blast_wave -> effectN( 1 ).percent();
        base_aoe_multiplier *= 0.5;
        aoe = -1;
    }

    virtual void init() override
    {
        mage_spell_t::init();

        // NOTE: Cooldown missing from tooltip since WoD beta build 18379
        cooldown -> duration = timespan_t::from_seconds( 25.0 );
        cooldown -> charges = 2;
    }
};

// Blazing Speed Spell ============================================================

struct blazing_speed_t: public mage_spell_t
{
  blazing_speed_t( mage_t* p, const std::string& options_str ):
    mage_spell_t( "blazing_speed", p, p -> talents.blazing_speed )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    p() -> buffs.blazing_speed -> trigger();
  }
};

// Blink Spell ==============================================================

struct blink_t : public mage_spell_t
{
  blink_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "blink", p, p -> find_class_spell( "Blink" ) )
  {
    parse_options( options_str );

    harmful = false;
    ignore_false_positive = true;
    base_teleport_distance = 20;
    base_teleport_distance += p -> glyphs.blink -> effectN( 1 ).base_value();
    if ( p -> glyphs.rapid_displacement -> ok() )
      cooldown -> charges = p -> glyphs.rapid_displacement -> effectN( 1 ).base_value();

    movement_directionality = MOVEMENT_OMNI;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( !p() -> glyphs.rapid_displacement -> ok() )
      player -> buffs.stunned -> expire();

    if ( p() -> perks.improved_blink -> ok() )
      p() -> buffs.improved_blink -> trigger();
  }
};

// Blizzard Spell ===========================================================

struct blizzard_shard_t : public mage_spell_t
{
  blizzard_shard_t( mage_t* p ) :
    mage_spell_t( "blizzard_shard", p, p -> find_class_spell( "Blizzard" ) -> effectN( 2 ).trigger() )
  {
    aoe = -1;
    background = true;
    ground_aoe = true;
  }

  // Override damage type because Blizzard is considered a DOT
  dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_OVER_TIME;
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> perks.improved_blizzard -> ok() )
      {
        p() -> cooldowns.frozen_orb
            -> adjust( -10.0 * p() -> perks.improved_blizzard
                                   -> effectN( 1 ).time_value() );
      }

      double fof_proc_chance = p() -> spec.fingers_of_frost
                                   -> effectN( 2 ).percent();
      p() -> buffs.fingers_of_frost
          -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance);
    }
  }
};

struct blizzard_t : public mage_spell_t
{
  blizzard_shard_t* shard;

  blizzard_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "blizzard", p, p -> find_specialization_spell( "Blizzard" ) ),
    shard( new blizzard_shard_t( p ) )
  {
    parse_options( options_str );

    channeled    = true;
    hasted_ticks = false;
    may_miss     = false;
    ignore_false_positive = true;

    add_child( shard );
  }

  void tick( dot_t* d ) override
  {
    mage_spell_t::tick( d );

    shard -> execute();
  }
};

// Cold Snap Spell ==========================================================

struct cold_snap_t : public mage_spell_t
{
  cold_snap_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "cold_snap", p, p -> talents.cold_snap )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    p() -> resource_gain( RESOURCE_HEALTH, p() -> resources.base[ RESOURCE_HEALTH ] * p() -> talents.cold_snap -> effectN( 2 ).percent() );

    // NOTE: Include Frost Nova and Ice Block CD reset here if implemented

    if ( p() -> specialization() == MAGE_ARCANE )
    {
      p() -> cooldowns.presence_of_mind -> reset( false );
    }
    else if ( p() -> specialization() == MAGE_FIRE )
    {
      p() -> cooldowns.dragons_breath -> reset( false );
    }
    else if ( p() -> specialization() == MAGE_FROST )
    {
      p() -> cooldowns.cone_of_cold -> reset( false );
    }
  }
};

// Combustion Spell =========================================================

struct combustion_t : public mage_spell_t
{
  const spell_data_t* tick_spell;

  combustion_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "combustion", p, p -> find_class_spell( "Combustion" ) ),
    // The "tick" portion of spell is specified in the DBC data in an alternate version of Combustion
    tick_spell( p -> find_spell( 83853, "combustion_dot" ) )
  {
    parse_options( options_str );

    base_tick_time = tick_spell -> effectN( 1 ).period();
    dot_duration   = tick_spell -> duration();
    tick_may_crit  = true;

    if ( p -> sets.has_set_bonus( SET_CASTER, T14, B4 ) )
    {
      cooldown -> duration = data().cooldown() * 0.8;
    }

    if ( p -> glyphs.combustion -> ok() )
    {
      dot_duration *= ( 1.0 + p -> glyphs.combustion -> effectN( 1 ).percent() );
      cooldown -> duration *= 1.0 + p -> glyphs.combustion -> effectN( 2 ).percent();
      base_dd_multiplier *= 1.0 + p -> glyphs.combustion -> effectN( 3 ).percent();
    }
  }

  void init() override
  {
    mage_spell_t::init();

    update_flags &= ~(STATE_CRIT | STATE_TGT_CRIT);
  }

  action_state_t* new_state() override
  { return new residual_periodic_state_t( this, target ); }

  virtual double calculate_tick_amount( action_state_t* state,
                                        double dmg_multiplier ) const override
  {
    double amount = 0.0;

    if ( dot_t* d = find_dot( state -> target ) )
    {

      const residual_periodic_state_t* dps_t =
        debug_cast<const residual_periodic_state_t*>( d -> state );
      amount += dps_t -> tick_amount;
    }

    state -> result_raw = amount;

    if ( state -> result == RESULT_CRIT )
      amount *= 1.0 + total_crit_bonus();
    else if ( state -> result == RESULT_MULTISTRIKE )
      amount *= composite_multistrike_multiplier( state );
    else if ( state -> result == RESULT_MULTISTRIKE_CRIT )
      amount *= composite_multistrike_multiplier( state ) *
                ( 1.0 + total_crit_bonus() );


    amount *= dmg_multiplier;

    state -> result_total = amount;

    return amount;
  }

  virtual void trigger_dot( action_state_t* s ) override
  {
    mage_td_t* this_td = td( s -> target );

    dot_t* ignite_dot     = this_td -> dots.ignite;
    dot_t* combustion_dot = this_td -> dots.combustion;

    if ( ignite_dot -> is_ticking() )
    {
      mage_spell_t::trigger_dot( s );

      residual_periodic_state_t* combustion_dot_state_t = debug_cast<residual_periodic_state_t*>( combustion_dot -> state );
      const residual_periodic_state_t* ignite_dot_state_t = debug_cast<const residual_periodic_state_t*>( ignite_dot -> state );

      // Combustion tooltip: "equal to X seconds of Ignite's current damage
      // done over <Combustion's duration>". Compute this by using Ignite's
      // tick damage, number of seconds, and Combustion's base duration.
      double base_duration = tick_spell -> duration().total_seconds();
      combustion_dot_state_t -> tick_amount = ignite_dot_state_t -> tick_amount *
                                              data().effectN( 1 ).base_value()  / base_duration;
    }
  }

  virtual void execute() override
  {
    p() -> cooldowns.inferno_blast -> reset( false );

    mage_spell_t::execute();
  }

  double last_tick_factor( const dot_t* /* d */, const timespan_t& /* time_to_tick */,
                           const timespan_t& /* duration */ ) const override
  {
    return 1.0;
  }
};

// Comet Storm Spell =======================================================

struct comet_storm_projectile_t : public mage_spell_t
{
  comet_storm_projectile_t( mage_t* p) :
    mage_spell_t( "comet_storm_projectile", p, p -> find_spell( 153596 ) )
  {
    aoe = -1;
    split_aoe_damage = true;
    background = true;
    school = SCHOOL_FROST;
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
  }
};

struct comet_storm_t : public mage_spell_t
{
  comet_storm_projectile_t* projectile;

  comet_storm_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "comet_storm", p, p -> talents.comet_storm ),
    projectile( new comet_storm_projectile_t( p ) )
  {
    parse_options( options_str );

    may_miss = false;

    base_tick_time    = timespan_t::from_seconds( 0.2 );
    dot_duration      = timespan_t::from_seconds( 1.2 );
    hasted_ticks      = false;

    dynamic_tick_action = true;
    add_child( projectile );
  }

  virtual void execute() override
  {
    mage_spell_t::execute();
    projectile -> execute();
  }

  void tick( dot_t* d ) override
  {
    mage_spell_t::tick( d );
    projectile -> execute();
  }
};

// Cone of Cold Spell =======================================================

struct cone_of_cold_t : public mage_spell_t
{
  cone_of_cold_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "cone_of_cold", p, p -> find_class_spell( "Cone of Cold" ) )
  {
    parse_options( options_str );
    aoe = -1;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();
    p() -> buffs.frozen_thoughts -> expire();
  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> glyphs.cone_of_cold -> ok() )
    {
      am *=  1.0 + p() -> glyphs.cone_of_cold -> effectN( 1 ).percent();
    }

    if ( p() -> buffs.frozen_thoughts -> up() )
    {
      am *= ( 1.0 + p() -> buffs.frozen_thoughts -> data().effectN( 1 ).percent() );
    }

    return am;
  }
};

// Counterspell Spell =======================================================

struct counterspell_t : public mage_spell_t
{
  counterspell_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "counterspell", p, p -> find_class_spell( "Counterspell" ) )
  {
    parse_options( options_str );
    may_miss = may_crit = false;
    may_proc_missiles = false;
    ignore_false_positive = true;
  }

  virtual bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Dragon's Breath Spell ====================================================

struct dragons_breath_t : public mage_spell_t
{
  dragons_breath_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "dragons_breath", p, p -> find_class_spell( "Dragon's Breath" ) )
  {
    parse_options( options_str );
    aoe = -1;
  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> glyphs.dragons_breath -> ok() )
    {
      am *=  1.0 + p() -> glyphs.dragons_breath -> effectN( 1 ).percent();
    }

    return am;
  }
};

// Evocation Spell ==========================================================

class evocation_t : public mage_spell_t
{
  int arcane_charges;

public:
  evocation_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "evocation", p,  p -> find_class_spell( "Evocation" ) ),
    arcane_charges( 0 )
  {
    parse_options( options_str );

    base_tick_time    = timespan_t::from_seconds( 2.0 );
    channeled         = true;
    dot_duration      = data().duration();
    harmful           = false;
    hasted_ticks      = false;
    tick_zero         = true;
    ignore_false_positive = true;

    if ( p -> perks.improved_evocation -> ok() )
    {
      cooldown -> duration += p -> perks.improved_evocation
                                -> effectN( 1 ).time_value();
    }
  }

  virtual void tick( dot_t* d ) override
  {
    mage_spell_t::tick( d );

    double mana_gain = p() -> resources.max[ RESOURCE_MANA ] *
                       (data().effectN( 1 ).percent() +
                        p() -> spec.arcane_charge_passive
                            -> effectN( 3 ).percent());

    if ( p() -> passives.nether_attunement -> ok() )
    {
      mana_gain /= p() -> cache.spell_haste();
    }

    mana_gain *= 1.0 + arcane_charges *
                       p() -> spec.arcane_charge -> effectN( 4 ).percent() *
                       ( 1.0 + p() -> sets.set( SET_CASTER, T15, B4)
                                   -> effectN( 1 ).percent() );

    p() -> resource_gain( RESOURCE_MANA, mana_gain, p() -> gains.evocation );
  }

  virtual void last_tick( dot_t* d ) override
  {
    mage_t& p = *this -> p();

    mage_spell_t::last_tick( d );
    if ( p.sets.has_set_bonus( MAGE_ARCANE, T17, B2 ) )
      p.buffs.arcane_affinity -> trigger();
  }

  virtual void execute() override
  {
    mage_t& p = *this -> p();

    arcane_charges = p.buffs.arcane_charge -> check();
    p.buffs.arcane_charge -> expire();
    mage_spell_t::execute();
  }
};

// Fire Blast Spell =========================================================

struct fire_blast_t : public mage_spell_t
{
  fire_blast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "fire_blast", p, p -> find_class_spell( "Fire Blast" ) )
  {
    parse_options( options_str );
  }
};

// Fireball Spell ===========================================================

struct fireball_t : public mage_spell_t
{
  fireball_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "fireball", p, p -> find_class_spell( "Fireball" ) )
  {
    parse_options( options_str );
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = mage_spell_t::travel_time();
    return std::min( timespan_t::from_seconds( 0.75 ), t );
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> perks.enhanced_pyrotechnics -> ok() )
      {
        if ( s -> result == RESULT_CRIT )
        {
          p() -> buffs.enhanced_pyrotechnics -> expire();
        }
        else
        {
          p() -> buffs.enhanced_pyrotechnics -> trigger();
        }
      }

      trigger_hot_streak( s );

      if ( p() -> talents.kindling -> ok() && s -> result == RESULT_CRIT )
      {
        p() -> cooldowns.combustion
            -> adjust( -1000 * p() -> talents.kindling
                                   -> effectN( 1 ).time_value() );
      }
    }

    if ( result_is_hit_or_multistrike( s -> result) )
    {
      if ( p() -> talents.unstable_magic -> ok() )
      {
        trigger_unstable_magic( s );
      }
      trigger_ignite( s );
    }
  }

  double composite_target_crit( player_t* target ) const override
  {
    double c = mage_spell_t::composite_target_crit( target );

    // Fire PvP 4pc set bonus
    if ( td( target ) -> debuffs.firestarter -> check() )
    {
      c += td( target ) -> debuffs.firestarter
                        -> data().effectN( 1 ).percent();
    }

    return c;
  }

  virtual double composite_crit() const override
  {
    double c = mage_spell_t::composite_crit();

      c += p() -> buffs.enhanced_pyrotechnics -> stack() *
           p() -> perks.enhanced_pyrotechnics -> effectN( 1 ).trigger()
                                              -> effectN( 1 ).percent();

    return c;
  }

  double composite_crit_multiplier() const override
  {
    double m = mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }
};

// Flamestrike Spell ========================================================

struct flamestrike_t : public mage_spell_t
{
  flamestrike_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "flamestrike", p, p -> find_specialization_spell( "Flamestrike" ) )
  {
    parse_options( options_str );

    if ( p -> perks.improved_flamestrike -> ok() )
    {
      cooldown -> duration = timespan_t::zero();
    }
    ignore_false_positive = true;

    aoe = -1;
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit_or_multistrike( s -> result ) )
    {
      trigger_ignite( s );
    }
  }
};

// Frost Bomb Spell ===============================================================

struct frost_bomb_explosion_t : public mage_spell_t
{
  frost_bomb_explosion_t( mage_t* p ) :
    mage_spell_t( "frost_bomb_explosion", p, p -> find_spell( 113092 ) )
  {
    background = true;
    callbacks = false;
    radius = data().effectN( 2 ).radius_max();

    aoe = -1;
    parse_effect_data( data().effectN( 1 ) );
    base_aoe_multiplier *= data().effectN( 2 ).sp_coeff() / data().effectN( 1 ).sp_coeff();
  }

  virtual resource_e current_resource() const override
  { return RESOURCE_NONE; }

  virtual timespan_t travel_time() const override
  { return timespan_t::zero(); }
};

struct frost_bomb_t : public mage_spell_t
{
  frost_bomb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frost_bomb", p, p -> talents.frost_bomb )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> last_bomb_target != nullptr &&
           p() -> last_bomb_target != execute_state -> target )
      {
        td( p() -> last_bomb_target ) -> dots.frost_bomb -> cancel();
        td( p() -> last_bomb_target ) -> debuffs.frost_bomb -> expire();
      }
      p() -> last_bomb_target = execute_state -> target;
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.frost_bomb -> trigger();
    }
  }
};

// Frostbolt Spell ==========================================================

struct frostbolt_t : public mage_spell_t
{
  double bf_multistrike_bonus;
  timespan_t enhanced_frostbolt_duration;
  cooldown_t* enhanced_frostbolt_cooldown;
  // Icicle stats variable to parent icicle damage to Frostbolt, instead of
  // clumping FB/FFB icicle damage together in reports.
  stats_t* icicle;

  frostbolt_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frostbolt", p, p -> find_specialization_spell( "Frostbolt" ) ),
    bf_multistrike_bonus( 0.0 ),
    enhanced_frostbolt_duration( p -> find_spell( 157648 ) -> duration() ),
    icicle( p -> get_stats( "icicle_fb" ) )
  {
    parse_options( options_str );

    stats -> add_child( icicle );
    icicle -> school = school;
    icicle -> action_list.push_back( p -> icicle );

    enhanced_frostbolt_cooldown = p -> get_cooldown( "enhanced_frostbolt" );
  }

  virtual void schedule_execute( action_state_t* execute_state ) override
  {
    if ( p() -> perks.enhanced_frostbolt -> ok() &&
         enhanced_frostbolt_cooldown -> up() )
    {
      p() -> buffs.enhanced_frostbolt -> trigger();
    }

    mage_spell_t::schedule_execute( execute_state );
  }

  virtual int schedule_multistrike( action_state_t* s, dmg_e dmg_type, double tick_multiplier ) override
  {
    int sm = mage_spell_t::schedule_multistrike( s, dmg_type, tick_multiplier );

    bf_multistrike_bonus = sm * p() -> spec.brain_freeze
                                    -> effectN( 2 ).percent();

    return sm;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t cast = mage_spell_t::execute_time();

    if ( p() -> buffs.enhanced_frostbolt -> check() )
      cast *= 1.0 + p() -> perks.enhanced_frostbolt -> effectN( 1 ).time_value().total_seconds() /
                  base_execute_time.total_seconds();
    if ( p() -> buffs.ice_shard -> up() )
      cast *= 1.0 + ( p() -> buffs.ice_shard -> stack() * p() -> buffs.ice_shard -> data().effectN( 1 ).percent() );
    return cast;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( p() -> buffs.enhanced_frostbolt -> up() )
    {
      p() -> buffs.enhanced_frostbolt -> expire();
      enhanced_frostbolt_cooldown -> start( enhanced_frostbolt_duration );
    }

    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost
                                   -> effectN( 1 ).percent();

      fof_proc_chance += p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 3 ).percent();

      if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
      {
        fof_proc_chance *= 1.2;
      }

      p() -> buffs.fingers_of_frost
          -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );
      p() -> buffs.brain_freeze
          -> trigger( 1, buff_t::DEFAULT_VALUE(),
                      p() -> spec.brain_freeze -> effectN( 1 ).percent() +
                      bf_multistrike_bonus );

      if ( p() -> shatterlance )
      {
        p() -> buffs.shatterlance -> trigger();
      }
    }

    p() -> buffs.frozen_thoughts -> expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit_or_multistrike( s -> result ) )
    {
      if ( p() -> talents.unstable_magic -> ok() )
      {
        trigger_unstable_magic( s );
      }

      trigger_icicle_gain( s, icicle );
    }

    if ( result_is_hit( s -> result ) &&
         ! p() -> pets.water_elemental -> is_sleeping() )
    {
      pets::water_elemental_pet_td_t* we_td = p() -> pets.water_elemental -> get_target_data( execute_state -> target );
      if ( we_td -> water_jet -> up() )
      {
        p() -> buffs.fingers_of_frost
            -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
      }
    }

  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    if ( p() -> buffs.frozen_thoughts -> up() )
    {
      am *= 1.0 + p() -> buffs.frozen_thoughts -> data().effectN( 1 ).percent();
    }

    if ( p() -> buffs.ice_shard -> up() )
    {
      am *= 1.0 + ( p() -> buffs.ice_shard -> stack() * p() -> buffs.ice_shard -> data().effectN( 2 ).percent() );
    }

    return am;
  }
};

// Frostfire Bolt Spell =====================================================

// Cast by Frost T16 4pc bonus when Brain Freeze FFB is cast
struct frigid_blast_t : public mage_spell_t
{
  frigid_blast_t( mage_t* p ) :
    mage_spell_t( "frigid_blast", p, p -> find_spell( 145264 ) )
  {
    background = true;
    may_crit = true;
  }
};

struct frostfire_bolt_t : public mage_spell_t
{
  frigid_blast_t* frigid_blast;
  // Icicle stats variable to parent icicle damage to Frostfire Bolt, instead of
  // clumping FB/FFB icicle damage together in reports.
  stats_t* icicle;

  frostfire_bolt_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frostfire_bolt", p, p -> find_spell( 44614 ) ),
    frigid_blast( nullptr ),
    icicle( nullptr )
  {
    parse_options( options_str );

    if ( p -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
    {
      frigid_blast = new frigid_blast_t( p );
      add_child( frigid_blast );
    }

    if ( p -> specialization() == MAGE_FROST )
    {
      icicle = p -> get_stats( "icicle_ffb" );
      stats -> add_child( icicle );
      icicle -> school = school;
      icicle -> action_list.push_back( p -> icicle );
    }
  }

  virtual double cost() const override
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return 0.0;

    return mage_spell_t::cost();
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.brain_freeze -> check() )
      return timespan_t::zero();

    return mage_spell_t::execute_time();
  }
  virtual void execute() override
  {
    // Brain Freeze treats the target as frozen
    frozen = p() -> buffs.brain_freeze -> up() != 0;
    mage_spell_t::execute();


    if ( result_is_hit( execute_state -> result ) )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost
                                   -> effectN( 1 ).percent();

      if ( p() -> buffs.icy_veins -> up() && p() -> glyphs.icy_veins -> ok() )
      {
        fof_proc_chance *= 1.2;
      }

      p() -> buffs.fingers_of_frost
          -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );
    }

    p() -> buffs.frozen_thoughts -> expire();
    if ( p() -> buffs.brain_freeze -> check() &&
         p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
    {
      p() -> buffs.frozen_thoughts -> trigger();
    }

    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B4 ) &&
         rng().roll( p() -> sets.set( SET_CASTER, T16, B4 )
                         -> effectN( 2 ).percent() ) )
    {
      frigid_blast -> schedule_execute();
    }

    p() -> buffs.brain_freeze -> decrement();
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = mage_spell_t::travel_time();
    return std::min( timespan_t::from_seconds( 0.75 ), t );
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> specialization() == MAGE_FIRE )
    {
      if ( p() -> perks.enhanced_pyrotechnics -> ok() )
      {
        if ( s -> result == RESULT_CRIT )
        {
          p() -> buffs.enhanced_pyrotechnics -> expire();
        }
        else
        {
          p() -> buffs.enhanced_pyrotechnics -> trigger();
        }
      }

      trigger_hot_streak( s );

      if ( p() -> talents.kindling -> ok() && s -> result == RESULT_CRIT )
      {
        p() -> cooldowns.combustion
            -> adjust( -1000 * p() -> talents.kindling
                                   -> effectN( 1 ).time_value() );
      }
    }

    if ( result_is_hit_or_multistrike( s -> result ) )
    {
      if ( p() -> specialization() == MAGE_FIRE )
      {
        trigger_ignite( s );
      }
      else if ( p() -> specialization() == MAGE_FROST )
      {
        trigger_icicle_gain( s, icicle );
      }

      if ( p() -> talents.unstable_magic -> ok() )
      {
        trigger_unstable_magic( s );
      }
    }
  }

  virtual double composite_crit() const override
  {
    double c = mage_spell_t::composite_crit();

    if ( p() -> perks.enhanced_pyrotechnics -> ok() )
    {
      c += p() -> buffs.enhanced_pyrotechnics -> stack() *
           p() -> perks.enhanced_pyrotechnics -> effectN( 1 ).trigger()
                                              -> effectN( 1 ).percent();
    }

    return c;
  }

  double composite_target_crit( player_t* target ) const override
  {
    double c = mage_spell_t::composite_target_crit( target );

    // 4pc Fire PvP bonus
    if ( td( target ) -> debuffs.firestarter -> check() )
    {
      c += td( target ) -> debuffs.firestarter -> data().effectN( 1 ).percent();
    }

    return c;
  }
  virtual double composite_crit_multiplier() const override
  {
    double m = mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    // 2T16
    if ( p() -> buffs.frozen_thoughts -> up() )
    {
      am *= ( 1.0 + p() -> buffs.frozen_thoughts -> data().effectN( 1 ).percent() );
    }

    if ( p() -> buffs.brain_freeze -> up() )
    {
      double bf_multiplier = p() -> spec.brain_freeze -> effectN( 3 ).percent();

      if ( p() -> sets.has_set_bonus( MAGE_FROST, T18, B2 ) )
      {
        bf_multiplier += p() -> sets.set( MAGE_FROST, T18, B2 )
                             -> effectN( 2 ).percent();
      }

      am *= 1.0 + bf_multiplier;
    }

    return am;
  }

};

// Frozen Orb Spell =========================================================

struct frozen_orb_bolt_t : public mage_spell_t
{
  frozen_orb_bolt_t( mage_t* p ) :
    mage_spell_t( "frozen_orb_bolt", p, p -> find_class_spell( "Frozen Orb" ) -> ok() ? p -> find_spell( 84721 ) : spell_data_t::not_found() )
  {
    aoe = -1;
    background = true;
    dual = true;
    cooldown -> duration = timespan_t::zero(); // dbc has CD of 6 seconds
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      double fof_proc_chance = p() -> spec.fingers_of_frost
                                   -> effectN( 2 ).percent();
      p() -> buffs.fingers_of_frost
          -> trigger( 1, buff_t::DEFAULT_VALUE(), fof_proc_chance );
    }
  }

  // Override damage type because Frozen Orb is considered a DOT
  dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_OVER_TIME;
  }
};

struct frozen_orb_t : public mage_spell_t
{
  frozen_orb_bolt_t* frozen_orb_bolt;
  frozen_orb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "frozen_orb", p, p -> find_class_spell( "Frozen Orb" ) ),
    frozen_orb_bolt( new frozen_orb_bolt_t( p ) )
  {
    parse_options( options_str );

    hasted_ticks = false;
    base_tick_time    = timespan_t::from_seconds( 1.0 );
    dot_duration      = data().duration();
    add_child( frozen_orb_bolt );
    may_miss       = false;
    may_crit       = false;
  }

  void tick( dot_t* d ) override
  {
    mage_spell_t::tick( d );
    // "travel time" reduction of ticks based on distance from target - set on the side of less ticks lost.
    if ( d -> current_tick <= ( d -> num_ticks - util::round( ( ( player -> current.distance - 16.0 ) / 16.0 ), 0 ) ) )
    {
    frozen_orb_bolt -> target = d -> target;
    frozen_orb_bolt -> execute();
    }
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( p() -> sets.has_set_bonus( MAGE_FROST, T17, B4 ) )
    {
      p() -> buffs.frost_t17_4pc -> trigger();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    p() -> buffs.fingers_of_frost
        -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
  }
};

// Ice Floes Spell ==========================================================

struct ice_floes_t : public mage_spell_t
{
  cooldown_t* icd;

  ice_floes_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "ice_floes", p, p -> talents.ice_floes )
  {
    parse_options( options_str );
    may_miss = may_crit = harmful = false;
    may_multistrike = 0;
    trigger_gcd = timespan_t::zero();

    cooldown -> charges = data().charges();
    cooldown -> duration = data().charge_cooldown();

    icd = p -> get_cooldown( "ice_floes_icd" );
  }

  bool ready() override
  {
    if ( icd -> down() )
    {
      return false;
    }

    return mage_spell_t::ready();
  }

  void execute() override
  {
    mage_spell_t::execute();

    icd -> start( data().internal_cooldown() );

    p() -> buffs.ice_floes -> trigger( 1 );
  }
};

// Ice Lance Spell ==========================================================

struct ice_lance_t : public mage_spell_t
{
  int frozen_orb_action_id;
  frost_bomb_explosion_t* frost_bomb_explosion;

  double shatterlance_effect;

  ice_lance_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "ice_lance", p, p -> find_class_spell( "Ice Lance" ) ),
    frost_bomb_explosion( nullptr ),
    shatterlance_effect( 0.0 )
  {
    parse_options( options_str );

    if ( p -> glyphs.splitting_ice -> ok() )
    {
      aoe = 1 + p -> glyphs.splitting_ice -> effectN( 1 ).base_value();
      base_aoe_multiplier = p -> glyphs.splitting_ice
                              -> effectN( 2 ).percent();
    }

    if ( p -> shatterlance )
    {
      const spell_data_t* data = p -> shatterlance -> driver();
      shatterlance_effect = data -> effectN( 1 ).average( p -> shatterlance -> item );
      shatterlance_effect /= 100.0;
    }

    if ( p -> talents.frost_bomb -> ok() )
    {
      frost_bomb_explosion = new frost_bomb_explosion_t( p );
    }
  }

  virtual void execute() override
  {
    // Ice Lance treats the target as frozen with FoF up
    frozen = p() -> buffs.fingers_of_frost -> up() != 0;

    mage_spell_t::execute();

    if ( p() -> talents.thermal_void -> ok() && p() -> buffs.icy_veins -> check() )
    {
      p() -> buffs.icy_veins -> extend_duration( p(), timespan_t::from_seconds( p() -> talents.thermal_void -> effectN( 1 ).base_value() ) );
    }

    if ( p() -> sets.has_set_bonus( MAGE_FROST, T17, B2 ) &&
         frozen_orb_action_id >= 0 &&
         p() -> get_active_dots( frozen_orb_action_id ) >= 1)
    {
      p() -> buffs.ice_shard -> trigger();
    }

    // Begin casting all Icicles at the target, beginning 0.25 seconds after the
    // Ice Lance cast with remaining Icicles launching at intervals of 0.75
    // seconds, both values adjusted by haste. Casting continues until all
    // Icicles are gone, including new ones that accumulate while they're being
    // fired. If target dies, Icicles stop. If Ice Lance is cast again, the
    // current sequence is interrupted and a new one begins.

    p() -> buffs.fingers_of_frost -> decrement();
    p() -> buffs.frozen_thoughts -> expire();
    p() -> trigger_icicle( execute_state, true, target );
  }

  virtual int schedule_multistrike( action_state_t* s, dmg_e dmg_type, double tick_multiplier ) override
  {
    // Prevent splitting ice cleaves onto PC from multistriking
    if ( s -> target == p() -> pets.prismatic_crystal &&
         s -> chain_target > 0 )
    {
      return 0;
    }

    return mage_spell_t::schedule_multistrike( s, dmg_type, tick_multiplier );
  }

  virtual void impact( action_state_t* s ) override
  {
    // Splitting Ice does not cleave onto Prismatic Crystal
    if ( s -> target == p() -> pets.prismatic_crystal &&
         s -> chain_target > 0 )
    {
      return;
    }

    mage_spell_t::impact( s );

    if ( td( s -> target ) -> debuffs.frost_bomb -> check() &&
         frozen && result_is_hit( s -> result ) )
    {
      frost_bomb_explosion -> target = s -> target;
      frost_bomb_explosion -> execute();
    }
  }

  virtual void init() override
  {
    mage_spell_t::init();

    frozen_orb_action_id = p() -> find_action_id( "frozen_orb" );
  }

  virtual double action_multiplier() const override
  {
    double am = mage_spell_t::action_multiplier();

    if ( frozen )
    {
      am *= 1.0 + data().effectN( 2 ).percent();
    }

    if ( p() -> buffs.fingers_of_frost -> up() )
    {
      am *= 1.0 + p() -> buffs.fingers_of_frost -> data().effectN( 2 ).percent();
    }

    if ( p() -> sets.has_set_bonus( SET_CASTER, T14, B2 ) )
    {
      am *= 1.12;
    }

    if ( p() -> buffs.frozen_thoughts -> up() )
    {
      am *= ( 1.0 + p() -> buffs.frozen_thoughts -> data().effectN( 1 ).percent() );
    }

    if ( p() -> buffs.shatterlance -> up() )
    {
      am *= 1.0 + shatterlance_effect;
    }

    return am;
  }
};

// Ice Nova Spell ==========================================================

struct ice_nova_t : public mage_spell_t
{
  ice_nova_t( mage_t* p, const std::string& options_str ) :
     mage_spell_t( "ice_nova", p, p -> talents.ice_nova )
  {
    parse_options( options_str );
    base_multiplier *= 1.0 + p -> talents.ice_nova -> effectN( 1 ).percent();
    base_aoe_multiplier *= 0.5;
    aoe = -1;
  }

  virtual void init() override
  {
    mage_spell_t::init();

    // NOTE: Cooldown missing from tooltip since WoD beta build 18379
    cooldown -> duration = timespan_t::from_seconds( 25.0 );
    cooldown -> charges = 2;
  }
};

// Icy Veins Spell ==========================================================

struct icy_veins_t : public mage_spell_t
{
  icy_veins_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "icy_veins", p, p -> find_class_spell( "Icy Veins" ) )
  {
    check_spec( MAGE_FROST );
    parse_options( options_str );
    harmful = false;

    if ( player -> sets.has_set_bonus( SET_CASTER, T14, B4 ) )
    {
      cooldown -> duration *= 0.5;
    }
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    p() -> buffs.icy_veins -> trigger();
  }
};

// Inferno Blast Spell ======================================================

struct inferno_blast_t : public mage_spell_t
{
  int max_spread_targets;
  double pyrosurge_chance;
  flamestrike_t* pyrosurge_flamestrike;
  inferno_blast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "inferno_blast", p,
                  p -> find_class_spell( "Inferno Blast" ) ),
    pyrosurge_chance( 0.0 )
  {
    parse_options( options_str );
    cooldown -> duration = timespan_t::from_seconds( 8.0 );
    radius = 10;
    cooldown -> duration += p -> sets.set( MAGE_FIRE, T17, B2 ) -> effectN( 1 ).time_value();

    max_spread_targets = data().effectN( 2 ).base_value();
    if ( p -> glyphs.inferno_blast -> ok() )
    {
      max_spread_targets += p -> glyphs.inferno_blast
                              -> effectN( 1 ).base_value();
    }
    if ( p -> perks.improved_inferno_blast -> ok() )
    {
      max_spread_targets += p -> perks.improved_inferno_blast
                              -> effectN( 1 ).base_value();
    }

    if ( p -> pyrosurge )
    {
      const spell_data_t* data = p -> pyrosurge -> driver();
      pyrosurge_chance = data -> effectN( 1 ).average( p -> pyrosurge -> item );
      pyrosurge_chance /= 100.0;

      pyrosurge_flamestrike = new flamestrike_t( p, options_str );
      pyrosurge_flamestrike -> background = true;
      pyrosurge_flamestrike -> callbacks = false;
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      mage_td_t* this_td = td( s -> target );

      dot_t* combustion_dot  = this_td -> dots.combustion,
           * ignite_dot      = this_td -> dots.ignite,
           * living_bomb_dot = this_td -> dots.living_bomb,
           * pyroblast_dot   = this_td -> dots.pyroblast;

      int spread_remaining = max_spread_targets;
      std::vector< player_t* >& tl = target_list();

      // Randomly choose spread targets
      std::random_shuffle( tl.begin(), tl.end() );

      for ( size_t i = 0, actors = tl.size();
            i < actors && spread_remaining > 0; i++ )
      {
        player_t* t = tl[ i ];

        if ( t == s -> target )
          continue;
        //Skip PC if trying to cleave to it.
        if ( t == p() -> pets.prismatic_crystal )
          continue;

        // Combustion does not spread to already afflicted targets
        // Source : http://goo.gl/tCsaqr
        if ( combustion_dot -> is_ticking() &&
             !td( t ) -> dots.combustion -> is_ticking()  )
        {
          combustion_dot -> copy( t, DOT_COPY_CLONE );
        }

        if ( ignite_dot -> is_ticking() )
        {
          if ( td( t ) -> dots.ignite -> is_ticking() )
          {
            // When the spread target has an ignite, add source ignite
            // remaining bank damage of the  to the spread target's ignite,
            // and reset ignite's tick count.
            // TODO: Patch 6.0,2 inferno blast behavior exhibits a delay when
            //       acquiring remaining number of igniteticks.
            //       Verify implementation when bug is fixed.
            residual_periodic_state_t* dot_state =
              debug_cast<residual_periodic_state_t*>( ignite_dot -> state );
            double ignite_bank = dot_state -> tick_amount *
                                 ignite_dot -> ticks_left();

            residual_action::trigger( p() -> active_ignite, t, ignite_bank);
          }
          else
          {
            ignite_dot -> copy( t, DOT_COPY_START );
          }
        }

        if ( living_bomb_dot -> is_ticking() )
        {
          living_bomb_dot -> copy( t, DOT_COPY_CLONE );
        }

        if ( pyroblast_dot -> is_ticking() )
        {
          pyroblast_dot -> copy( t, DOT_COPY_CLONE );
        }

        spread_remaining--;
      }

      if ( p() -> pyrosurge && p() -> rng().roll( pyrosurge_chance ) )
      {
        pyrosurge_flamestrike -> target = s -> target;
        pyrosurge_flamestrike -> execute();
      }

      trigger_hot_streak( s );

      if ( s -> result == RESULT_CRIT && p() -> talents.kindling -> ok() )
      {
        p() -> cooldowns.combustion
            -> adjust( -1000 * p() -> talents.kindling
                                   -> effectN( 1 ).time_value() );
      }
    }

    if ( result_is_hit_or_multistrike( s -> result ) )
    {
      trigger_ignite( s );
    }
  }

  // Inferno Blast always crits
  virtual double composite_crit() const override
  { return 1.0; }

  virtual double composite_target_crit( player_t* ) const override
  { return 0.0; }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B4 ) &&
         p() -> level() <= 90 )
    {
      p() -> buffs.fiery_adept -> trigger();
    }
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_explosion_t : public mage_spell_t
{
  living_bomb_explosion_t( mage_t* p ) :
    mage_spell_t( "living_bomb_explosion", p, p -> find_spell( 44461 ) )
  {
    aoe = -1;
    radius = 10;
    background = true;
  }

  virtual resource_e current_resource() const override
  { return RESOURCE_NONE; }
};

struct living_bomb_t : public mage_spell_t
{
  living_bomb_explosion_t* explosion;

  living_bomb_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "living_bomb", p, p -> talents.living_bomb ),
    explosion( new living_bomb_explosion_t( p ) )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      dot_t* dot = get_dot( s -> target );
      if ( dot -> is_ticking() && dot -> remains() < dot_duration * 0.3 )
      {
        explosion -> target = s -> target;
        explosion -> execute();
      }
    }
    mage_spell_t::impact( s );
  }

  void last_tick( dot_t* d ) override
  {
    mage_spell_t::last_tick( d );

    explosion -> target = d -> target;
    explosion -> execute();
  }
};

// Meteor Spell ========================================================

// Implementation details from Celestalon:
// http://blue.mmo-champion.com/topic/318876-warlords-of-draenor-theorycraft-discussion/#post301
// Meteor is split over a number of spell IDs, some of which don't seem to be
// used for anything useful:
// - Meteor (id=153561) is the talent spell, the driver
// - Meteor (id=153564) is the initial impact damage
// - Meteor Burn (id=155158) is the ground effect tick damage
// - Meteor Burn (id=175396) provides the tooltip's burn duration (8 seconds),
//   but doesn't match in game where we only see 7 ticks over 7 seconds.
// - Meteor (id=177345) contains the time between cast and impact
// None of these specify the 1 second falling duration given by Celestalon, so
// we're forced to hardcode it.

struct meteor_burn_t : public mage_spell_t
{
  meteor_burn_t( mage_t* p, int targets ) :
    mage_spell_t( "meteor_burn", p, p -> find_spell( 155158 ) )
  {
    background = true;
    aoe = targets;
    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    spell_power_mod.tick = 0;
    dot_duration = timespan_t::zero();
    radius = p -> find_spell( 153564 ) -> effectN( 1 ).radius_max();
    ground_aoe = true;
  }

  // Override damage type because Meteor Burn is considered a DOT
  dmg_e amount_type( const action_state_t* /* state */, bool /* periodic */ ) const override
  {
    return DMG_OVER_TIME;
  }
};

struct meteor_impact_t: public mage_spell_t
{
  meteor_impact_t( mage_t* p, int targets ):
    mage_spell_t( "meteor_impact", p, p -> find_spell( 153564 ) )
  {
    background = true;
    aoe = targets;
    split_aoe_damage = true;
    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    ground_aoe = true;
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit_or_multistrike( s -> result ) )
    {
      trigger_ignite( s );
    }
  }
};

struct meteor_t : public mage_spell_t
{
  int targets;
  meteor_impact_t* meteor_impact;
  meteor_burn_t* meteor_burn;
  timespan_t meteor_delay;
  timespan_t actual_tick_time;
  meteor_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "meteor", p, p -> find_talent_spell( "Meteor") ),
    targets( -1 ),
    meteor_impact( new meteor_impact_t( p, targets ) ),
    meteor_burn( new meteor_burn_t( p, targets ) ),
    meteor_delay( p -> find_spell( 177345 ) -> duration() )
  {
    add_option( opt_int( "targets", targets ) );
    parse_options( options_str );
    hasted_ticks = false;
    callbacks = false;
    may_multistrike = 0;
    add_child( meteor_impact );
    add_child( meteor_burn );
    dot_duration = p -> find_spell( 175396 ) -> duration() - p -> find_spell( 153564 ) -> duration();
    actual_tick_time = p -> find_spell( 155158 ) -> effectN( 1 ).period();
    school = SCHOOL_FIRE;
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t impact_time = meteor_delay * p() ->  composite_spell_haste();
    timespan_t meteor_spawn = std::max( timespan_t::zero(),
                                        impact_time - timespan_t::from_seconds( 1.0 ) );

    return meteor_spawn;
  }

  void impact( action_state_t* s ) override
  {
    base_tick_time = timespan_t::from_seconds( 2 ); // Yep. Don't hate. Need to make the dot tick 1 second after impact.
    mage_spell_t::impact( s );
    meteor_impact -> target = s -> target;
    meteor_impact -> execute();
    base_tick_time = actual_tick_time;
  }

  void tick( dot_t* d ) override
  {
    mage_spell_t::tick( d );
    meteor_burn -> target = d -> target;
    meteor_burn -> execute();
  }
};

// Mirror Image Spell =======================================================

struct mirror_image_t : public mage_spell_t
{
  mirror_image_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "mirror_image", p, p -> find_talent_spell( "Mirror Image" ) )
  {
    parse_options( options_str );
    dot_duration = timespan_t::zero();
    harmful = false;
  }

  bool init_finished() override
  {
    for ( unsigned i = 0; i < sizeof_array( p() -> pets.mirror_images ); i++ )
    {
      if ( ! p() -> pets.mirror_images[ i ] )
      {
        continue;
      }

      stats -> add_child( p() -> pets.mirror_images[ i ] -> get_stats( "arcane_blast" ) );
      stats -> add_child( p() -> pets.mirror_images[ i ] -> get_stats( "fire_blast" ) );
      stats -> add_child( p() -> pets.mirror_images[ i ] -> get_stats( "fireball" ) );
      stats -> add_child( p() -> pets.mirror_images[ i ] -> get_stats( "frostbolt" ) );
    }

    return mage_spell_t::init_finished();
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( p() -> pets.mirror_images[ 0 ] )
    {
      for ( unsigned i = 0; i < sizeof_array( p() -> pets.mirror_images ); i++ )
      {
        p() -> pets.mirror_images[ i ] -> summon( data().duration() );
      }
    }
  }
};

// Nether Tempest AoE Spell ====================================================
struct nether_tempest_aoe_t: public mage_spell_t
{
  nether_tempest_aoe_t( mage_t* p ) :
    mage_spell_t( "nether_tempest_aoe", p, p -> find_spell( 114954 ) )
  {
    aoe = -1;
    background = true;
  }

  virtual resource_e current_resource() const override
  { return RESOURCE_NONE; }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.3 );
  }
};

// Nether Tempest Spell ===========================================================
struct nether_tempest_t : public mage_spell_t
{
  nether_tempest_aoe_t* nether_tempest_aoe;

  nether_tempest_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "nether_tempest", p, p -> talents.nether_tempest ),
    nether_tempest_aoe( new nether_tempest_aoe_t( p ) )
  {
    parse_options( options_str );
    add_child( nether_tempest_aoe );
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> last_bomb_target != nullptr &&
           p() -> last_bomb_target != execute_state -> target )
      {
        td( p() -> last_bomb_target ) -> dots.nether_tempest -> cancel();
      }
      p() -> last_bomb_target = execute_state -> target;
    }
  }

  virtual void tick( dot_t* d ) override
  {
    if ( p() -> regen_type == REGEN_DYNAMIC )
    {
      p() -> do_dynamic_regen();
    }

    mage_spell_t::tick( d );

    action_state_t* aoe_state = nether_tempest_aoe -> get_state( d -> state );
    aoe_state -> target = d -> target;

    nether_tempest_aoe -> schedule_execute( aoe_state );
  }

  double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = mage_spell_t::composite_persistent_multiplier( state );

    m *= 1.0 +  p() -> buffs.arcane_charge -> stack() *
                p() -> spec.arcane_charge -> effectN( 1 ).percent();

    return m;
  }
};

// Pyroblast Spell ==========================================================

//Mage T18 2pc Fire Set Bonus
struct conjure_phoenix_t : public mage_spell_t
{
  conjure_phoenix_t( mage_t* p ) :
    mage_spell_t( "conjure_phoenix", p, p -> find_spell( 186181 ) )
  {
    background = true;
    callbacks = false;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( p() -> sets.has_set_bonus( MAGE_FIRE, T18, B4 ) )
      p() -> buffs.icarus_uprising -> trigger();
  }

};

struct pyroblast_t : public mage_spell_t
{
  bool is_hot_streak,
       dot_is_hot_streak;

  conjure_phoenix_t* conjure_phoenix;

  pyroblast_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "pyroblast", p, p -> find_class_spell( "Pyroblast" ) ),
    is_hot_streak( false ), dot_is_hot_streak( false ),
    conjure_phoenix( nullptr )
  {
    parse_options( options_str );

    if ( p -> sets.has_set_bonus( MAGE_FIRE, T18, B2 ) )
    {
      conjure_phoenix = new conjure_phoenix_t( p );
      add_child( conjure_phoenix );
    }
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    mage_spell_t::schedule_execute( state );

    p() -> buffs.pyroblast -> up();
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.pyroblast -> check() || p() -> buffs.pyromaniac -> check() )
    {
      return timespan_t::zero();
    }

    return mage_spell_t::execute_time();
  }

  virtual double cost() const override
  {
    if ( p() -> buffs.pyroblast -> check() || p() -> buffs.pyromaniac -> check() )
      return 0.0;

    return mage_spell_t::cost();
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( p() -> buffs.pyroblast -> check() && p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
    {
      p() -> buffs.potent_flames -> trigger();
    }

    is_hot_streak = p() -> buffs.pyroblast -> check() != 0;

    p() -> buffs.pyroblast -> expire();
    p() -> buffs.fiery_adept -> expire();
  }

  virtual timespan_t travel_time() const override
  {
    timespan_t t = mage_spell_t::travel_time();
    return std::min( t, timespan_t::from_seconds( 0.75 ) );
  }

  virtual void impact( action_state_t* s ) override
  {
    dot_is_hot_streak = is_hot_streak;

    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( s -> result == RESULT_CRIT && p() -> talents.kindling -> ok() )
      {
        p() -> cooldowns.combustion
            -> adjust( -1000 * p() -> talents.kindling
                                   -> effectN( 1 ).time_value()  );
      }

      trigger_hot_streak( s );

      if ( p() -> sets.has_set_bonus( MAGE_FIRE, PVP, B4 ) && is_hot_streak )
      {
        td( s -> target ) -> debuffs.firestarter -> trigger();
      }

      if ( p() -> sets.has_set_bonus( MAGE_FIRE, T18, B2 ) &&
           rng().roll( p() -> sets.set( MAGE_FIRE, T18, B2 ) -> proc_chance() ) )
      {
         conjure_phoenix -> schedule_execute();
      }
    }

    if ( result_is_hit_or_multistrike( s -> result) )
    {
      trigger_ignite( s );
    }

  }

  virtual double composite_crit_multiplier() const override
  {
    double m = mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual double composite_crit() const override
  {
    double c = mage_spell_t::composite_crit();

    c += p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 2 ).percent();

    if ( p() -> buffs.fiery_adept -> up() || p() -> buffs.pyromaniac -> up() )
      c += 1.0;

    return c;
  }

  virtual double action_da_multiplier() const override
  {
    double am = mage_spell_t::action_da_multiplier();

    if ( p() -> buffs.pyroblast -> up() )
    {
      am *= 1.0 + p() -> buffs.pyroblast -> data().effectN( 3 ).percent();
    }

    return am;
  }

  virtual double action_ta_multiplier() const override
  {
    double am = mage_spell_t::action_ta_multiplier();

    if ( dot_is_hot_streak )
    {
      am *= 1.0 + p() -> buffs.pyroblast -> data().effectN( 3 ).percent();
    }

    return am;
  }

  void reset() override
  {
    mage_spell_t::reset();

    is_hot_streak = false;
    dot_is_hot_streak = false;
  }
};

// Rune of Power ============================================================

struct rune_of_power_t : public mage_spell_t
{
  rune_of_power_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "rune_of_power", p, p -> talents.rune_of_power )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    // Assume they're always in it
    p() -> distance_from_rune = 0;
    p() -> buffs.rune_of_power -> trigger();
  }
};

// Scorch Spell =============================================================

struct scorch_t : public mage_spell_t
{
  scorch_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "scorch", p, p -> find_specialization_spell( "Scorch" ) )
  {
    parse_options( options_str );

    consumes_ice_floes = false;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( p() -> perks.improved_scorch -> ok() )
    {
      p() -> buffs.improved_scorch -> trigger();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result) )
    {
      trigger_hot_streak( s );
    }

    if ( result_is_hit_or_multistrike( s -> result) )
    {
      trigger_ignite( s );
    }
  }

  double composite_crit_multiplier() const override
  {
    double m = mage_spell_t::composite_crit_multiplier();

    m *= 1.0 + p() -> spec.critical_mass -> effectN( 1 ).percent();

    return m;
  }

  virtual bool usable_moving() const override
  { return true; }

  // delay 0.25s the removal of heating up on non-critting scorch
  virtual void expire_heating_up() override
  {
    mage_t* p = this -> p();
    if ( sim -> log ) sim -> out_log << "Heating up delay by 0.25s";
    p -> buffs.heating_up -> expire( timespan_t::from_millis( 250 ) );
  }
};

// Slow Spell ===============================================================

struct slow_t : public mage_spell_t
{
  slow_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "slow", p, p -> find_class_spell( "Slow" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs.slow -> trigger();
    }
  }
};

// Supernova Spell ==========================================================

struct supernova_t : public mage_spell_t
{
  supernova_t( mage_t* p, const std::string& options_str ) :
     mage_spell_t( "supernova", p, p -> talents.supernova )
  {
    parse_options( options_str );
    aoe = -1;
    base_multiplier *= 1.0 + p -> talents.supernova -> effectN( 1 ).percent();
    base_aoe_multiplier *= 0.5;
  }

  virtual void init() override
  {
    mage_spell_t::init();

    // NOTE: Cooldown missing from tooltip since WoD beta build 18379
    cooldown -> duration = timespan_t::from_seconds( 25.0 );
    cooldown -> charges = 2;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) &&
         execute_state -> n_targets > 1 )
    {
      // NOTE: Supernova AOE effect causes secondary trigger chance for AM
      p() -> buffs.arcane_missiles -> trigger();
    }
  }
};



// Time Warp Spell ==========================================================

struct time_warp_t : public mage_spell_t
{
  time_warp_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "time_warp", p, p -> find_class_spell( "Time Warp" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> buffs.exhaustion -> check() || p -> is_pet() )
        continue;

      p -> buffs.bloodlust -> trigger(); // Bloodlust and Timewarp are the same
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready() override
  {
    if ( sim -> overrides.bloodlust )
      return false;

    if ( player -> buffs.exhaustion -> check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Water Elemental Spell ====================================================

struct summon_water_elemental_t : public mage_spell_t
{
  summon_water_elemental_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "water_elemental", p, p -> find_class_spell( "Summon Water Elemental" ) )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    mage_spell_t::execute();

    p() -> pets.water_elemental -> summon();
  }

  virtual bool ready() override
  {
    if ( ! mage_spell_t::ready() )
      return false;

    return ! ( p() -> pets.water_elemental && ! p() -> pets.water_elemental -> is_sleeping() );
  }
};

// Prismatic Crystal Spell =========================================================

struct prismatic_crystal_t : public mage_spell_t
{
  prismatic_crystal_t( mage_t* p, const std::string& options_str ) :
    mage_spell_t( "prismatic_crystal", p, p -> find_talent_spell( "Prismatic Crystal" ) )
  {
    parse_options( options_str );
    may_miss = may_crit = harmful = callbacks = false;
    ignore_false_positive = true;
    action_skill = 1;
  }

  void execute() override
  {
    mage_spell_t::execute();

    p() -> pets.prismatic_crystal -> summon( data().duration() );
  }
};

// ================================================================================
// Mage Custom Actions
// ================================================================================


// Choose Target Action ============================================================

struct choose_target_t : public action_t
{
  bool check_selected;
  player_t* selected_target;

  // Infinite loop protection
  timespan_t last_execute;

  std::string target_name;

  choose_target_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "choose_target", p ),
    check_selected( false ), selected_target( nullptr ),
    last_execute( timespan_t::min() )
  {
    add_option( opt_string( "name", target_name ) );
    add_option( opt_bool( "check_selected", check_selected ) );
    parse_options( options_str );

    radius = range = -1.0;
    trigger_gcd = timespan_t::zero();

    harmful = may_miss = may_crit = callbacks = false;
    ignore_false_positive = true;
    action_skill = 1;
  }

  bool init_finished() override
  {
    if ( ! target_name.empty() && ! util::str_compare_ci( target_name, "default" ) )
    {
      selected_target = player -> actor_by_name_str( target_name );
    }
    else
      selected_target = player -> target;

    return action_t::init_finished();
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    mage_t* p = debug_cast<mage_t*>( player );
    action_t::available_targets( tl );

    if ( p -> pets.prismatic_crystal )
    {
      if ( target != p -> pets.prismatic_crystal && !p -> pets.prismatic_crystal -> is_sleeping() )
        tl.push_back( p -> pets.prismatic_crystal );
    }

    return tl.size();
  }

  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }

  block_result_e calculate_block_result( action_state_t* ) const override
  { return BLOCK_RESULT_UNBLOCKED; }

  void execute() override
  {
    action_t::execute();

    // Don't do anything if selected target is sleeping
    if ( ! selected_target || selected_target -> is_sleeping() )
    {
      return;
    }

    mage_t* p = debug_cast<mage_t*>( player );
    assert( ! target_if_expr || ( selected_target == select_target_if_target() ) );

    if ( sim -> current_time() == last_execute )
    {
      sim -> errorf( "%s choose_target infinite loop detected (due to no time passing between executes) at '%s'",
        p -> name(), signature_str.c_str() );
      sim -> cancel_iteration();
      sim -> cancel();
      return;
    }

    last_execute = sim -> current_time();

    if ( sim -> debug )
      sim -> out_debug.printf( "%s swapping target from %s to %s", player -> name(), p -> current_target -> name(), selected_target -> name() );

    p -> current_target = selected_target;

    // Invalidate target caches
    for ( size_t i = 0, end = p -> action_list.size(); i < end; i++ )
      p -> action_list[i] -> target_cache.is_valid = false;
    }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( target_if_mode != TARGET_IF_NONE )
    {
      selected_target = select_target_if_target();
      if ( selected_target == nullptr )
      {
        return false;
      }
    }

    // Safeguard stupidly against breaking the sim.
    if ( selected_target -> is_sleeping() )
    {
      // Reset target to default actor target if we're still targeting a dead selected target
      if ( p -> current_target == selected_target )
        p -> current_target = p -> target;

      return false;
    }

    if ( p -> current_target == selected_target )
      return false;

    player_t* original_target = nullptr;
    if ( check_selected )
    {
      if ( target != selected_target )
        original_target = target;

      target = selected_target;
    }
    else
      target = p -> current_target;

    bool rd = action_t::ready();

    if ( original_target )
      target = original_target;

    return rd;
  }

  void reset() override
  {
    action_t::reset();
    last_execute = timespan_t::min();
  }
};

// Combustion Pyroblast Chaining Switch Action ==========================================================

struct start_pyro_chain_t : public action_t
{
  start_pyro_chain_t( mage_t* p, const std::string& options_str ):
    action_t( ACTION_USE, "start_pyro_chain", p )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
    ignore_false_positive = true;
    action_skill = 1;
  }

  virtual void execute() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    bool success = p -> pyro_chain.enable( sim -> current_time() );
    if ( !success )
    {
      sim -> errorf( "%s start_pyro_chain infinite loop detected (no time passing between executes) at '%s'",
        p -> name(), signature_str.c_str() );
      sim -> cancel_iteration();
      sim -> cancel();
      return;
    }
  }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( p -> pyro_chain.on() )
    {
      return false;
    }

    return action_t::ready();
  }
};

struct stop_pyro_chain_t : public action_t
{
  stop_pyro_chain_t( mage_t* p, const std::string& options_str ):
     action_t( ACTION_USE, "stop_pyro_chain", p )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
    ignore_false_positive = true;
    action_skill = 1;
  }

  virtual void execute() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    bool success = p -> pyro_chain.disable( sim -> current_time() );
    if ( !success )
    {
      sim -> errorf( "%s stop_pyro_chain infinite loop detected (no time passing between executes) at '%s'",
        p -> name(), signature_str.c_str() );
      sim -> cancel_iteration();
      sim -> cancel();
      return;
    }
  }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( !p -> pyro_chain.on() )
    {
      return false;
    }

    return action_t::ready();
  }
};

// Arcane Mage "Burn" State Switch Action =====================================

struct start_burn_phase_t : public action_t
{
  start_burn_phase_t( mage_t* p, const std::string& options_str ):
    action_t( ACTION_USE, "start_burn_phase", p )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
    ignore_false_positive = true;
    action_skill = 1;
  }

  virtual void execute() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    bool success = p -> burn_phase.enable( sim -> current_time() );
    if ( !success )
    {
      sim -> errorf( "%s start_burn_phase infinite loop detected (no time passing between executes) at '%s'",
        p -> name(), signature_str.c_str() );
      sim -> cancel_iteration();
      sim -> cancel();
      return;
    }
  }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( p -> burn_phase.on() )
    {
      return false;
    }

    return action_t::ready();
  }
};

struct stop_burn_phase_t : public action_t
{
  stop_burn_phase_t( mage_t* p, const std::string& options_str ):
     action_t( ACTION_USE, "stop_burn_phase", p )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
    ignore_false_positive = true;
    action_skill = 1;
  }

  virtual void execute() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    bool success = p -> burn_phase.disable( sim -> current_time() );
    if ( !success )
    {
      sim -> errorf( "%s stop_burn_phase infinite loop detected (no time passing between executes) at '%s'",
        p -> name(), signature_str.c_str() );
      sim -> cancel_iteration();
      sim -> cancel();
      return;
    }
  }

  bool ready() override
  {
    mage_t* p = debug_cast<mage_t*>( player );

    if ( !p -> burn_phase.on() )
    {
      return false;
    }

    return action_t::ready();
  }
};

// Unstable Magic =============================================================

struct unstable_magic_explosion_t : public mage_spell_t
{
  unstable_magic_explosion_t( mage_t* p ) :
    mage_spell_t( "unstable_magic_explosion", p, p -> talents.unstable_magic )
  {
    may_miss = may_dodge = may_parry = may_crit = may_block = false;
    may_multistrike = callbacks = false;
    aoe = -1;
    base_costs[ RESOURCE_MANA ] = 0;
    trigger_gcd = timespan_t::zero();
    background = true;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    // For now, PC doubledips on UM
    if ( target == p() -> pets.prismatic_crystal )
      return p() -> pets.prismatic_crystal
                 -> composite_player_vulnerability( school );

    return 1.0;
  }

  virtual void init() override
  {
    mage_spell_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_TGT_MUL_DA;
  }

  virtual void execute() override
  {
    base_dd_max *= data().effectN( 4 ).percent();
    base_dd_min *= data().effectN( 4 ).percent();

    mage_spell_t::execute();
  }
};

void mage_spell_t::trigger_unstable_magic( action_state_t* s )
{
  double um_proc_rate;
  switch ( p() -> specialization() )
  {
    case MAGE_ARCANE:
      um_proc_rate = p() -> unstable_magic_explosion
                       -> data().effectN( 1 ).percent();
      break;
    case MAGE_FROST:
      um_proc_rate = p() -> unstable_magic_explosion
                       -> data().effectN( 2 ).percent();
      break;
    case MAGE_FIRE:
      um_proc_rate = p() -> unstable_magic_explosion
                       -> data().effectN( 3 ).percent();
      break;
    default:
      um_proc_rate = p() -> unstable_magic_explosion
                       -> data().effectN( 3 ).percent();
      break;
  }

  if ( p() -> rng().roll( um_proc_rate ) )
  {
    p() -> unstable_magic_explosion -> target = s -> target;
    p() -> unstable_magic_explosion -> base_dd_max = s -> result_amount;
    p() -> unstable_magic_explosion -> base_dd_min = s -> result_amount;
    p() -> unstable_magic_explosion -> execute();
  }
}

// Proxy cast Water Jet Action ================================================

struct water_jet_t : public action_t
{
  pets::water_elemental_pet_t::water_jet_t* action;

  water_jet_t( mage_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "water_jet", p ), action( nullptr )
  {
    parse_options( options_str );

    may_miss = may_crit = callbacks = false;
    dual = true;
    trigger_gcd = timespan_t::zero();
    ignore_false_positive = true;
    action_skill = 1;
  }

  void reset() override
  {
    action_t::reset();

    if ( ! action )
    {
      mage_t* m = debug_cast<mage_t*>( player );
      action = debug_cast<pets::water_elemental_pet_t::water_jet_t*>( m -> pets.water_elemental -> find_action( "water_jet" ) );
      assert( action );
      action -> autocast = false;
    }
  }

  void execute() override
  {
    mage_t* m = debug_cast<mage_t*>( player );
    action -> queued = true;
    // Interrupt existing cast
    if ( m -> pets.water_elemental -> executing )
    {
      m -> pets.water_elemental -> executing -> interrupt_action();
    }

    // Cancel existing (potential) player-ready event ..
    if ( m -> pets.water_elemental -> readying )
    {
      event_t::cancel( m -> pets.water_elemental -> readying );
    }

    // and schedule a new one immediately.
    m -> pets.water_elemental -> schedule_ready();
  }

  bool ready() override
  {
    mage_t* m = debug_cast<mage_t*>( player );
    if ( ! m -> perks.improved_water_ele -> ok() )
      return false;

    // Ensure that the Water Elemental's water_jet is ready. Note that this
    // skips the water_jet_t::ready() call, and simply checks the "base" ready
    // properties of the spell (most importantly, the cooldown). If normal
    // ready() was called, this would always return false, as queued = false,
    // before this action executes.
    if ( ! action -> spell_t::ready() )
      return false;

    // Don't re-execute if water jet is already queued
    if ( action -> queued == true )
      return false;

    return action_t::ready();
  }
};
// =====================================================================================
// Mage Specific Spell Overrides
// =====================================================================================


// Override T18 trinket procs so they are affected by mage multipliers
struct darklight_ray_t : public mage_spell_t
{
  darklight_ray_t( mage_t* p, const special_effect_t& effect ) :
    mage_spell_t( "darklight_ray", p, p -> find_spell( 183950 ) )
  {
    background = may_crit = true;
    callbacks = false;

    // Mage specific control
    may_proc_missiles = false;
    consumes_ice_floes = false;

    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item );

    aoe = -1;
  }
};

struct doom_nova_t : public mage_spell_t
{
  doom_nova_t( mage_t* p, const special_effect_t& effect ) :
    mage_spell_t( "doom_nova", p, p -> find_spell( 184075 ) )
  {
    background = may_crit = true;
    callbacks = false;

    // Mage specific control
    may_proc_missiles = false;
    consumes_ice_floes = false;

    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );

    aoe = -1;
  }
};

// Override Nithramus ring explosion to impact Prismatic Crystal

struct nithramus_t : public mage_spell_t
{
  double damage_coeff;
  nithramus_t( mage_t* p, const special_effect_t& effect ) :
    mage_spell_t( "nithramus", p, p -> find_spell( 187611 ) )
  {
    damage_coeff = data().effectN( 1 ).average( effect.item ) / 10000.0;

    background = split_aoe_damage = true;
    may_crit = callbacks = false;
    may_multistrike = 0;
    trigger_gcd = timespan_t::zero();
    aoe = -1;
    radius = 20;
    range = -1;
    travel_speed = 0.0;
  }

  void init() override
  {
    mage_spell_t::init();

    snapshot_flags = STATE_MUL_DA;
    update_flags = 0;
  }

  double composite_da_multiplier( const action_state_t* ) const override
  {
    return damage_coeff;
  }
};

} // namespace actions

namespace events {

struct icicle_event_t : public event_t
{
  mage_t* mage;
  player_t* target;
  icicle_data_t state;

  icicle_event_t( mage_t& m, const icicle_data_t& s, player_t* t, bool first = false ) :
    event_t( m ), mage( &m ), target( t ), state( s )
  {
    double cast_time = first ? 0.25 : 0.75;
    cast_time *= mage -> cache.spell_speed();

    add_event( timespan_t::from_seconds( cast_time ) );
  }
  virtual const char* name() const override
  { return "icicle_event"; }
  void execute() override
  {
    // If the target of the icicle is ded, stop the chain
    if ( target -> is_sleeping() )
    {
      if ( mage -> sim -> debug )
        mage -> sim -> out_debug.printf( "%s icicle use on %s (sleeping target), stopping",
            mage -> name(), target -> name() );
      mage -> icicle_event = nullptr;
      return;
    }

    actions::icicle_state_t* new_s = debug_cast<actions::icicle_state_t*>( mage -> icicle -> get_state() );
    new_s -> source = state.second;
    new_s -> target = target;

    mage -> icicle -> base_dd_min = mage -> icicle -> base_dd_max = state.first;

    // Immediately execute icicles so the correct damage is carried into the travelling icicle
    // object.
    mage -> icicle -> pre_execute_state = new_s;
    mage -> icicle -> execute();

    icicle_data_t new_state = mage -> get_icicle_object();
    if ( new_state.first > 0 )
    {
      mage -> icicle_event = new ( sim() ) icicle_event_t( *mage, new_state, target );
      if ( mage -> sim -> debug )
        mage -> sim -> out_debug.printf( "%s icicle use on %s (chained), damage=%f, total=%u",
                               mage -> name(), target -> name(), new_state.first, as<unsigned>( mage -> icicles.size() ) );
    }
    else
      mage -> icicle_event = nullptr;
  }
};

} // namespace events

// ==========================================================================
// Mage Character Definition
// ==========================================================================

// mage_td_t ================================================================

mage_td_t::mage_td_t( player_t* target, mage_t* mage ) :
  actor_target_data_t( target, mage ),
  dots( dots_t() ),
  debuffs( debuffs_t() )
{
  dots.combustion     = target -> get_dot( "combustion",     mage );
  dots.flamestrike    = target -> get_dot( "flamestrike",    mage );
  dots.frost_bomb     = target -> get_dot( "frost_bomb",     mage );
  dots.ignite         = target -> get_dot( "ignite",         mage );
  dots.living_bomb    = target -> get_dot( "living_bomb",    mage );
  dots.nether_tempest = target -> get_dot( "nether_tempest", mage );
  dots.pyroblast      = target -> get_dot( "pyroblast",      mage );
  dots.frozen_orb     = target -> get_dot( "frozen_orb",     mage );

  debuffs.frost_bomb = buff_creator_t( *this, "frost_bomb" ).spell( mage -> talents.frost_bomb );
  debuffs.firestarter = buff_creator_t( *this, "firestarter" ).chance( 1.0 ).duration( timespan_t::from_seconds( 10.0 ) );
  debuffs.water_jet = buff_creator_t( *this, "water_jet", source -> find_spell( 135029 ) )
                      .quiet( true )
                      .cd( timespan_t::zero() );
}

// mage_t::create_action ====================================================

action_t* mage_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  using namespace actions;

  // Arcane
  if ( name == "arcane_barrage"    ) return new          arcane_barrage_t( this, options_str );
  if ( name == "arcane_blast"      ) return new            arcane_blast_t( this, options_str );
  if ( name == "arcane_explosion"  ) return new        arcane_explosion_t( this, options_str );
  if ( name == "arcane_missiles"   ) return new         arcane_missiles_t( this, options_str );
  if ( name == "arcane_orb"        ) return new              arcane_orb_t( this, options_str );
  if ( name == "arcane_power"      ) return new            arcane_power_t( this, options_str );
  if ( name == "evocation"         ) return new               evocation_t( this, options_str );
  if ( name == "nether_tempest"    ) return new          nether_tempest_t( this, options_str );
  if ( name == "presence_of_mind"  ) return new        presence_of_mind_t( this, options_str );
  if ( name == "slow"              ) return new                    slow_t( this, options_str );
  if ( name == "supernova"         ) return new               supernova_t( this, options_str );

  if ( name == "start_burn_phase"  ) return new        start_burn_phase_t( this, options_str );
  if ( name == "stop_burn_phase"   ) return new         stop_burn_phase_t( this, options_str );

  // Fire
  if ( name == "blast_wave"        ) return new              blast_wave_t( this, options_str );
  if ( name == "combustion"        ) return new              combustion_t( this, options_str );
  if ( name == "dragons_breath"    ) return new          dragons_breath_t( this, options_str );
  if ( name == "fire_blast"        ) return new              fire_blast_t( this, options_str );
  if ( name == "fireball"          ) return new                fireball_t( this, options_str );
  if ( name == "flamestrike"       ) return new             flamestrike_t( this, options_str );
  if ( name == "inferno_blast"     ) return new           inferno_blast_t( this, options_str );
  if ( name == "living_bomb"       ) return new             living_bomb_t( this, options_str );
  if ( name == "meteor"            ) return new                  meteor_t( this, options_str );
  if ( name == "pyroblast"         ) return new               pyroblast_t( this, options_str );
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );

  if ( name == "start_pyro_chain"  ) return new        start_pyro_chain_t( this, options_str );
  if ( name == "stop_pyro_chain"   ) return new         stop_pyro_chain_t( this, options_str );

  // Frost
  if ( name == "blizzard"          ) return new                blizzard_t( this, options_str );
  if ( name == "comet_storm"       ) return new             comet_storm_t( this, options_str );
  if ( name == "frost_bomb"        ) return new              frost_bomb_t( this, options_str );
  if ( name == "frostbolt"         ) return new               frostbolt_t( this, options_str );
  if ( name == "frozen_orb"        ) return new              frozen_orb_t( this, options_str );
  if ( name == "ice_lance"         ) return new               ice_lance_t( this, options_str );
  if ( name == "ice_nova"          ) return new                ice_nova_t( this, options_str );
  if ( name == "icy_veins"         ) return new               icy_veins_t( this, options_str );
  if ( name == "water_elemental"   ) return new  summon_water_elemental_t( this, options_str );
  if ( name == "water_jet"         ) return new               water_jet_t( this, options_str );

  // Shared spells
  if ( name == "arcane_brilliance" ) return new       arcane_brilliance_t( this, options_str );
  if ( name == "blink"             ) return new                   blink_t( this, options_str );
  if ( name == "cone_of_cold"      ) return new            cone_of_cold_t( this, options_str );
  if ( name == "counterspell"      ) return new            counterspell_t( this, options_str );
  if ( name == "frostfire_bolt"    ) return new          frostfire_bolt_t( this, options_str );
  if ( name == "time_warp"         ) return new               time_warp_t( this, options_str );

  if ( name == "choose_target"     ) return new           choose_target_t( this, options_str );

  // Shared talents
  if ( name == "blazing_speed"     ) return new           blazing_speed_t( this, options_str );
  if ( name == "ice_floes"         ) return new               ice_floes_t( this, options_str );
  // TODO: Implement all T45 talents and enable freezing_grasp
  // if ( name == "freezing_grasp"    )
  if ( name == "cold_snap"         ) return new               cold_snap_t( this, options_str );
  if ( name == "mage_bomb"         )
  {
    if ( talents.frost_bomb -> ok() )
    {
      return new frost_bomb_t( this, options_str );
    }
    else if ( talents.living_bomb -> ok() )
    {
      return new living_bomb_t( this, options_str );
    }
    else if ( talents.nether_tempest -> ok() )
    {
      return new nether_tempest_t( this, options_str );
    }
  }
  if ( name == "mirror_image"      ) return new            mirror_image_t( this, options_str );
  if ( name == "rune_of_power"     ) return new           rune_of_power_t( this, options_str );
  if ( name == "prismatic_crystal" ) return new       prismatic_crystal_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_proc_action ===============================================

action_t* mage_t::create_proc_action( const std::string& name, const special_effect_t& effect )
{
  if ( util::str_compare_ci( name, "darklight_ray" ) ) return new actions::darklight_ray_t( this, effect );
  if ( util::str_compare_ci( name, "doom_nova" ) )     return new     actions::doom_nova_t( this, effect );
  if ( util::str_compare_ci( name, "nithramus" ) )     return new     actions::nithramus_t( this, effect );

  return nullptr;
}


// mage_t::create_pets ======================================================

void mage_t::create_pets()
{
  if ( specialization() == MAGE_FROST && find_action( "water_elemental" ) )
  {
    pets.water_elemental = new pets::water_elemental_pet_t( sim, this );
  }

  if ( talents.prismatic_crystal -> ok() && find_action( "prismatic_crystal" ) )
  {
    pets.prismatic_crystal = new pets::prismatic_crystal_t( sim, this );
  }

  if ( talents.mirror_image -> ok() && find_action( "mirror_image" ) )
  {
    for ( unsigned i = 0; i < sizeof_array( pets.mirror_images ); i++ )
    {
      pets.mirror_images[ i ] = new pets::mirror_image_pet_t( sim, this );
      if ( i > 0 )
      {
        pets.mirror_images[ i ] -> quiet = 1;
      }
    }
  }

  if ( sets.has_set_bonus( MAGE_ARCANE, T18, B2 ) )
  {
    for ( unsigned i = 0; i < sizeof_array( pets.temporal_heroes ); i++ )
    {
      pets.temporal_heroes[ i ] = new pets::temporal_hero_t( sim, this );
    }
  }
}

// mage_t::init_spells ======================================================

void mage_t::init_spells()
{
  player_t::init_spells();

  // Talents
  // Tier 15
  talents.evanesce           = find_talent_spell( "Evanesce"             );
  talents.blazing_speed      = find_talent_spell( "Blazing Speed"        );
  talents.ice_floes          = find_talent_spell( "Ice Floes"            );
  // Tier 30
  talents.alter_time         = find_talent_spell( "Alter Time"           );
  talents.flameglow          = find_talent_spell( "Flameglow"            );
  talents.ice_barrier        = find_talent_spell( "Ice Barrier"          );
  // Tier 45
  talents.ring_of_frost      = find_talent_spell( "Ring of Frost"        );
  talents.ice_ward           = find_talent_spell( "Ice Ward"             );
  talents.frostjaw           = find_talent_spell( "Frostjaw"             );
  // Tier 60
  talents.greater_invis      = find_talent_spell( "Greater Invisibility" );
  talents.cauterize          = find_talent_spell( "Cauterize"            );
  talents.cold_snap          = find_talent_spell( "Cold Snap"            );
  // Tier 75
  talents.nether_tempest     = find_talent_spell( "Nether Tempest"       );
  talents.living_bomb        = find_talent_spell( "Living Bomb"          );
  talents.frost_bomb         = find_talent_spell( "Frost Bomb"           );
  talents.unstable_magic     = find_talent_spell( "Unstable Magic"       );
  talents.supernova          = find_talent_spell( "Supernova"            );
  talents.blast_wave         = find_talent_spell( "Blast Wave"           );
  talents.ice_nova           = find_talent_spell( "Ice Nova"             );
  // Tier 90
  talents.mirror_image       = find_talent_spell( "Mirror Image"         );
  talents.rune_of_power      = find_talent_spell( "Rune of Power"        );
  talents.incanters_flow     = find_talent_spell( "Incanter's Flow"      );
  // Tier 100
  talents.overpowered        = find_talent_spell( "Overpowered"          );
  talents.kindling           = find_talent_spell( "Kindling"             );
  talents.thermal_void       = find_talent_spell( "Thermal Void"         );
  talents.prismatic_crystal  = find_talent_spell( "Prismatic Crystal"    );
  talents.arcane_orb         = find_talent_spell( "Arcane Orb"           );
  talents.meteor             = find_talent_spell( "Meteor"               );
  talents.comet_storm        = find_talent_spell( "Comet Storm"          );

  // Passive Spells
  passives.nether_attunement = find_specialization_spell( "Nether Attunement" ); // BUG: Not in spell lists at present.
  passives.nether_attunement = ( find_spell( 117957 ) -> is_level( true_level ) ) ? find_spell( 117957 ) : spell_data_t::not_found();
  passives.shatter           = find_specialization_spell( "Shatter" );
  passives.frost_armor       = find_specialization_spell( "Frost Armor" );
  passives.mage_armor        = find_specialization_spell( "Mage Armor" );
  passives.molten_armor      = find_specialization_spell( "Molten Armor" );


  // Perks - Fire
  perks.enhanced_pyrotechnics  = find_perk_spell( "Enhanced Pyrotechnics"    );
  perks.improved_flamestrike   = find_perk_spell( "Improved Flamestrike"     );
  perks.improved_inferno_blast = find_perk_spell( "Improved Inferno Blast"   );
  perks.improved_scorch        = find_perk_spell( "Improved Scorch"          );
  // Perks - Arcane
  perks.enhanced_arcane_blast  = find_perk_spell( "Enhanced Arcane Blast"    );
  perks.improved_arcane_power  = find_perk_spell( "Improved Arcane Power"    );
  perks.improved_evocation     = find_perk_spell( "Improved Evocation"       );
  perks.improved_blink         = find_perk_spell( "Improved Blink"           );
  // Perks - Frost
  perks.enhanced_frostbolt     = find_perk_spell( "Enhanced Frostbolt"       );
  perks.improved_blizzard      = find_perk_spell( "Improved Blizzard"        );
  perks.improved_water_ele     = find_perk_spell( "Improved Water Elemental" );
  perks.improved_icy_veins     = find_perk_spell( "Improved Icy Veins"       );

  // Spec Spells
  spec.arcane_charge         = find_spell( 36032 );
  spec.arcane_charge_passive = find_spell( 114664 );

  spec.critical_mass         = find_specialization_spell( "Critical Mass"    );

  spec.brain_freeze          = find_specialization_spell( "Brain Freeze"     );
  spec.fingers_of_frost      = find_specialization_spell( "Fingers of Frost" );

  // Attunments
  spec.arcane_mind           = find_specialization_spell( "Arcane Mind"      );
  spec.ice_shards            = find_specialization_spell( "Ice Shards"       );
  spec.incineration          = find_specialization_spell( "Incineration"     );

  // Mastery
  spec.icicles               = find_mastery_spell( MAGE_FROST );
  spec.icicles_driver        = find_spell( 148012 );
  spec.ignite                = find_mastery_spell( MAGE_FIRE );
  spec.mana_adept            = find_mastery_spell( MAGE_ARCANE );

  // Glyphs
  glyphs.arcane_power       = find_glyph_spell( "Glyph of Arcane Power"       );
  glyphs.arcane_explosion   = find_glyph_spell( "Glyph of Arcane Explosion"   );
  glyphs.blink              = find_glyph_spell( "Glyph of Blink"              );
  glyphs.combustion         = find_glyph_spell( "Glyph of Combustion"         );
  glyphs.cone_of_cold       = find_glyph_spell( "Glyph of Cone of Cold"       );
  glyphs.dragons_breath     = find_glyph_spell( "Glyph of Dragon's Breath"    );
  glyphs.icy_veins          = find_glyph_spell( "Glyph of Icy Veins"          );
  glyphs.inferno_blast      = find_glyph_spell( "Glyph of Inferno Blast"      );
  glyphs.living_bomb        = find_glyph_spell( "Glyph of Living Bomb"        );
  glyphs.rapid_displacement = find_glyph_spell( "Glyph of Rapid Displacement" );
  glyphs.splitting_ice      = find_glyph_spell( "Glyph of Splitting Ice"      );

  // Active spells
  if ( spec.ignite -> ok()  )
    active_ignite = new actions::ignite_t( this );
  if ( spec.icicles -> ok() )
    icicle = new actions::icicle_t( this );
  if ( talents.unstable_magic )
    unstable_magic_explosion = new actions::unstable_magic_explosion_t( this );

  // RPPM
  rppm_pyromaniac.set_frequency( find_spell( 165459 ) -> real_ppm() );
  rppm_pyromaniac.set_initial_precombat_time( timespan_t::from_seconds( -120 ) );
  rppm_arcane_instability.set_frequency( find_spell( 165476 ) -> real_ppm() );
}

// mage_t::init_base ========================================================

void mage_t::init_base_stats()
{
  player_t::init_base_stats();

  base.spell_power_per_intellect = 1.0;

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;

  base.mana_regen_per_second = resources.base[ RESOURCE_MANA ] * 0.018;

  // Reduce fire mage distance to avoid proc munching at high haste. Or reduce distance if we're using Prismatic Crystal
  if ( specialization() == MAGE_FIRE || talents.prismatic_crystal  -> ok() )
    base.distance = 20;

  if ( race == RACE_ORC )
    pet_multiplier *= 1.0 + find_racial_spell( "Command" ) -> effectN( 1 ).percent();
}

// mage_t::init_buffs =======================================================

struct incanters_flow_t : public buff_t
{
  incanters_flow_t( mage_t* p ) :
    buff_t( buff_creator_t( p, "incanters_flow", p -> find_spell( 116267 ) ) // Buff is a separate spell
            .duration( p -> sim -> max_time * 3 ) // Long enough duration to trip twice_expected_event
            .period( p -> talents.incanters_flow -> effectN( 1 ).period() ) ) // Period is in the talent
  { }

  void bump( int stacks, double value ) override
  {
    int before_stack = current_stack;
    buff_t::bump( stacks, value );
    // Reverse direction if max stacks achieved before bump
    if ( before_stack == current_stack )
      reverse = true;
  }

  void decrement( int stacks, double value ) override
  {
    // This buff will never fade; reverse direction at 1 stack.
    // Buff uptime reporting _should_ work ok with this solution
    if ( current_stack > 1 )
      buff_t::decrement( stacks, value );
    else
      reverse = false;
  }
};


// Buff callback functions

static void frost_t17_4pc_fof_gain( buff_t* buff, int, int )
{
  mage_t* p = debug_cast<mage_t*>( buff -> player );

  p -> buffs.fingers_of_frost -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
  if ( p -> sim -> debug )
  {
    p -> sim -> out_debug.printf( "%s gains Fingers of Frost from 4T17",
                                  p -> name() );
  }
}


// mage_t::create_buffs =======================================================

void mage_t::create_buffs()
{
  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  // Arcane
  buffs.arcane_charge         = buff_creator_t( this, "arcane_charge", spec.arcane_charge );
  buffs.arcane_missiles       = buff_creator_t( this, "arcane_missiles", find_spell( 79683 ) )
                                  .chance( find_spell( 79684 ) -> proc_chance() );
  buffs.arcane_power          = buff_creator_t( this, "arcane_power", find_spell( 12042 ) )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  if ( glyphs.arcane_power -> ok () )
  {
    buffs.arcane_power -> buff_duration *= 1.0 + glyphs.arcane_power -> effectN( 1 ).percent();
  }
  buffs.presence_of_mind      = buff_creator_t( this, "presence_of_mind", find_spell( 12043 ) )
                                  .activated( true )
                                  .cd( timespan_t::zero() )
                                  .duration( timespan_t::zero() );
  buffs.improved_blink        = buff_creator_t( this, "improved_blink", perks.improved_blink )
                                  .default_value( perks.improved_blink -> effectN( 1 ).percent() );
  buffs.mage_armor            = stat_buff_creator_t( this, "mage_armor", find_spell( 6117 ) );
  buffs.profound_magic        = buff_creator_t( this, "profound_magic", find_spell( 145252 ) )
                                  .chance( sets.has_set_bonus( SET_CASTER, T16, B2 ) );
  buffs.arcane_affinity       = buff_creator_t( this, "arcane_affinity", find_spell( 166871 ))
                                  .chance( sets.has_set_bonus( MAGE_ARCANE, T17, B2 ) );
  buffs.arcane_instability    = buff_creator_t( this, "arcane_instability", find_spell( 166872 ) )
                                  .chance( sets.has_set_bonus( MAGE_ARCANE, T17, B4 ) );
  // 4T18 Temporal Power buff has no duration and stacks multiplicatively
  buffs.temporal_power        = buff_creator_t( this, "temporal_power", find_spell( 190623 ) )
                                  .max_stack( 10 );

  // Fire
  buffs.heating_up            = buff_creator_t( this, "heating_up",  find_spell( 48107 ) );
  buffs.molten_armor          = buff_creator_t( this, "molten_armor", find_spell( 30482 ) )
                                  .add_invalidate( CACHE_SPELL_CRIT );
  buffs.pyroblast             = buff_creator_t( this, "pyroblast",  find_spell( 48108 ) );
  buffs.enhanced_pyrotechnics = buff_creator_t( this, "enhanced_pyrotechnics", find_spell( 157644 ) );
  buffs.icarus_uprising       = buff_creator_t( this, "icarus_uprising", find_spell( 186170 ) )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                  .add_invalidate( CACHE_SPELL_HASTE );
  buffs.improved_scorch       = buff_creator_t( this, "improved_scorch", find_spell( 157633 ) )
                                  .default_value( perks.improved_scorch -> effectN( 1 ).percent() );
  buffs.potent_flames         = stat_buff_creator_t( this, "potent_flames", find_spell( 145254 ) )
                                  .chance( sets.has_set_bonus( SET_CASTER, T16, B2 ) );
  buffs.fiery_adept           = buff_creator_t( this, "fiery_adept", find_spell( 145261 ) )
                                  .chance( sets.has_set_bonus( SET_CASTER, T16, B4 ) );
  buffs.pyromaniac            = buff_creator_t( this, "pyromaniac", find_spell( 166868 ) )
                                  .chance( sets.has_set_bonus( MAGE_FIRE, T17, B4 ) );

  // Frost
  buffs.brain_freeze          = buff_creator_t( this, "brain_freeze", find_spell( 57761 ) );
  buffs.fingers_of_frost      = buff_creator_t( this, "fingers_of_frost", find_spell( 44544 ) )
                                  .max_stack( find_spell( 44544 ) -> max_stacks() +
                                              ( sets.has_set_bonus( MAGE_FROST, T18, B4 )? find_spell( 185971 ) -> effectN( 2 ).base_value() : 0 ) );
  buffs.frost_armor           = buff_creator_t( this, "frost_armor", find_spell( 7302 ) )
                                  .add_invalidate( CACHE_MULTISTRIKE );
  buffs.icy_veins             = buff_creator_t( this, "icy_veins", find_spell( 12472 ) );
  if ( glyphs.icy_veins -> ok () )
  {
    buffs.icy_veins -> add_invalidate( CACHE_MULTISTRIKE );
  }
  else
  {
    buffs.icy_veins -> add_invalidate( CACHE_SPELL_HASTE );
  }
  buffs.enhanced_frostbolt    = buff_creator_t( this, "enhanced_frostbolt", find_spell( 157646 ) );
  buffs.frozen_thoughts       = buff_creator_t( this, "frozen_thoughts", find_spell( 146557 ) );
  buffs.frost_t17_4pc         = buff_creator_t( this, "frost_t17_4pc", find_spell( 165470 ) )
                                  .duration( find_spell( 84714 ) -> duration() )
                                  .period( find_spell( 165470 ) -> effectN( 1 ).time_value() )
                                  .quiet( true )
                                  .tick_callback( frost_t17_4pc_fof_gain );
  buffs.ice_shard             = buff_creator_t( this, "ice_shard", find_spell( 166869 ) );
  buffs.shatterlance          = buff_creator_t( this, "shatterlance")
                                  .duration( timespan_t::from_seconds( 0.9 ) )
                                  .cd( timespan_t::zero() )
                                  .chance( 1.0 );

  // Talents
  buffs.blazing_speed         = buff_creator_t( this, "blazing_speed", talents.blazing_speed )
                                  .default_value( talents.blazing_speed -> effectN( 1 ).percent() );
  buffs.ice_floes             = buff_creator_t( this, "ice_floes", talents.ice_floes );
  buffs.incanters_flow        = new incanters_flow_t( this );
  buffs.rune_of_power         = buff_creator_t( this, "rune_of_power", find_spell( 116014 ) )
                                  .duration( find_spell( 116011 ) -> duration() )
                                  .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
}

// mage_t::init_gains =======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains.evocation              = get_gain( "evocation"              );
}

// mage_t::init_procs =======================================================

void mage_t::init_procs()
{
  player_t::init_procs();

  procs.test_for_crit_hotstreak = get_proc( "test_for_crit_hotstreak" );
  procs.crit_for_hotstreak      = get_proc( "crit_test_hotstreak"     );
  procs.hotstreak               = get_proc( "hotstreak"               );
}

// mage_t::init_uptimes =====================================================

void mage_t::init_benefits()
{
  player_t::init_benefits();

  for ( unsigned i = 0; i < sizeof_array( benefits.arcane_charge ); ++i )
  {
    benefits.arcane_charge[ i ] = get_benefit( "Arcane Charge " + util::to_string( i )  );
  }
}

// mage_t::init_stats =========================================================

void mage_t::init_stats()
{
  player_t::init_stats();

  // Cache Icy Veins haste multiplier for performance reasons
  double haste = buffs.icy_veins -> data().effectN( 1 ).percent();
  if ( perks.improved_icy_veins -> ok() )
  {
    haste += perks.improved_icy_veins -> effectN( 1 ).percent();
  }
  iv_haste = 1.0 / ( 1.0 + haste );

  // Register target reset callback here (anywhere later on than in constructor) so older GCCs are
  // happy
  sim -> target_non_sleeping_list.register_callback( current_target_reset_cb_t( this ) );
}

// mage_t::init_actions =====================================================

void mage_t::init_action_list()
{
  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat();

  switch ( specialization() )
  {
    case MAGE_ARCANE:
      apl_arcane();
      break;
    case MAGE_FROST:
      apl_frost();
      break;
    case MAGE_FIRE:
      apl_fire();
      break;
    default:
      apl_default(); // DEFAULT
      break;
  }

  // Default
  use_default_action_list = true;

  player_t::init_action_list();
}

// mage_t::has_t18_class_trinket ==============================================

bool mage_t::has_t18_class_trinket() const
{
  if ( specialization() == MAGE_ARCANE )
  {
    return wild_arcanist != nullptr;
  }
  else if ( specialization() == MAGE_FIRE )
  {
    return pyrosurge != nullptr;
  }
  else if ( specialization() == MAGE_FROST )
  {
    return shatterlance != nullptr;
  }

  return false;
}

//Pre-combat Action Priority List============================================

void mage_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  if( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";

    if ( true_level <= 85 )
      flask_action += "draconic_mind" ;
    else if ( true_level <= 90 )
      flask_action += "warm_sun" ;
    else
      flask_action += "greater_draenic_intellect_flask" ;

    precombat -> add_action( flask_action );
  }
    // Food
  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";

    if ( level() <= 85 )
      food_action += "seafood_magnifique_feast" ;
    else if ( level() <= 90 )
      food_action += "mogu_fish_stew" ;
    else if ( specialization() == MAGE_ARCANE && sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
      food_action += "buttered_sturgeon" ;
    else if ( specialization() == MAGE_ARCANE )
      food_action += "sleeper_sushi" ;
    else if ( specialization() == MAGE_FIRE )
      food_action += "pickled_eel" ;
    else
      food_action += "salty_squid_roll" ;

    precombat -> add_action( food_action );
  }

  // Arcane Brilliance
  precombat -> add_action( this, "Arcane Brilliance" );

  // Water Elemental
  if ( specialization() == MAGE_FROST )
    precombat -> add_action( "water_elemental" );

  // Snapshot Stats
  precombat -> add_action( "snapshot_stats" );

  // Level 90 talents
  precombat -> add_talent( this, "Rune of Power" );
  precombat -> add_talent( this, "Mirror Image" );

  //Potions
  if ( sim -> allow_potions && true_level >= 80 )
  {
    precombat -> add_action( get_potion_action() );
  }

  if ( specialization() == MAGE_ARCANE )
    precombat -> add_action( this, "Arcane Blast" );
  else if ( specialization() == MAGE_FIRE )
    precombat -> add_action( this, "Pyroblast" );
  else
  {
    precombat -> add_action( this, "Frostbolt", "if=!talent.frost_bomb.enabled" );
    precombat -> add_talent( this, "Frost Bomb" );
  }
}


// Util for using level appropriate potion

std::string mage_t::get_potion_action()
{
  std::string potion_action = "potion,name=";

  if ( true_level <= 85 )
    potion_action += "volcanic" ;
  else if ( true_level <= 90 )
    potion_action += "jade_serpent" ;
  else
    potion_action += "draenic_intellect" ;

  return potion_action;
}


// Arcane Mage Action List====================================================

void mage_t::apl_arcane()
{
  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"          );

  action_priority_list_t* movement            = get_action_priority_list( "movement"         );
  action_priority_list_t* init_burn           = get_action_priority_list( "init_burn"        );
  action_priority_list_t* init_crystal        = get_action_priority_list( "init_crystal"     );
  action_priority_list_t* crystal_sequence    = get_action_priority_list( "crystal_sequence" );
  action_priority_list_t* cooldowns           = get_action_priority_list( "cooldowns"        );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe"              );
  action_priority_list_t* burn                = get_action_priority_list( "burn"             );
  action_priority_list_t* conserve            = get_action_priority_list( "conserve"         );


  default_list -> add_action( this, "Counterspell",
                              "if=target.debuff.casting.react" );
  default_list -> add_action( "stop_burn_phase,if=prev_gcd.evocation&burn_phase_duration>gcd.max" );
  default_list -> add_talent( this, "Cold Snap",
                              "if=health.pct<30" );
  default_list -> add_action( this, "Time Warp",
                              "if=target.health.pct<25|time>5" );
  default_list -> add_action( "call_action_list,name=movement,if=raid_event.movement.exists" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<2*spell_haste" );
  default_list -> add_talent( this, "Mirror Image" );
  default_list -> add_talent( this, "Cold Snap",
                              "if=buff.presence_of_mind.down&cooldown.presence_of_mind.remains>75" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>=5" );
  default_list -> add_action( "call_action_list,name=init_burn,if=!burn_phase" );
  default_list -> add_action( "call_action_list,name=burn,if=burn_phase" );
  default_list -> add_action( "call_action_list,name=conserve" );


  movement -> add_action( this, "Blink",
                          "if=movement.distance>10" );
  movement -> add_talent( this, "Blazing Speed",
                          "if=movement.remains>0" );
  movement -> add_talent( this, "Ice Floes",
                          "if=buff.ice_floes.down&(raid_event.movement.distance>0|raid_event.movement.in<2*spell_haste)" );


  init_burn -> add_action( "start_burn_phase,if=buff.arcane_charge.stack>=4&(legendary_ring.cooldown.remains<gcd.max|legendary_ring.cooldown.remains>target.time_to_die+15|!legendary_ring.has_cooldown)&(cooldown.prismatic_crystal.up|!talent.prismatic_crystal.enabled)&(cooldown.arcane_power.up|(glyph.arcane_power.enabled&cooldown.arcane_power.remains>60))&(cooldown.evocation.remains-2*buff.arcane_missiles.stack*spell_haste-gcd.max*talent.prismatic_crystal.enabled)*0.75*(1-0.1*(cooldown.arcane_power.remains<5))*(1-0.1*(talent.nether_tempest.enabled|talent.supernova.enabled))*(10%action.arcane_blast.execute_time)<mana.pct-20-2.5*active_enemies*(9-active_enemies)+(cooldown.evocation.remains*1.8%spell_haste)",
                           "Regular burn with evocation" );
  init_burn -> add_action( "start_burn_phase,if=talent.prismatic_crystal.enabled&t18_class_trinket&equipped.124230&time<1",
                           "PC + ToSW + PoF opener burn" );
  init_burn -> add_action( "start_burn_phase,if=target.time_to_die*0.75*(1-0.1*(talent.nether_tempest.enabled|talent.supernova.enabled))*(10%action.arcane_blast.execute_time)*1.1<mana.pct-10+(target.time_to_die*1.8%spell_haste)",
                           "End of fight burn" );


  init_crystal -> add_talent( this, "Prismatic Crystal",
                              "if=t18_class_trinket&equipped.124230&time<2" );
  init_crystal -> add_action( "call_action_list,name=conserve,if=buff.arcane_charge.stack<4|(buff.arcane_missiles.react&debuff.mark_of_doom.remains>2*spell_haste+(target.distance%20))",
                              "Conditions for initiating Prismatic Crystal" );
  init_crystal -> add_action( this, "Arcane Missiles",
                              "if=buff.arcane_missiles.react&t18_class_trinket" );
  init_crystal -> add_talent( this, "Prismatic Crystal" );


  crystal_sequence -> add_action( "call_action_list,name=cooldowns",
                                  "Actions while Prismatic Crystal is active" );
  crystal_sequence -> add_talent( this, "Nether Tempest",
                                  "if=buff.arcane_charge.stack=4&!ticking&pet.prismatic_crystal.remains>8" );
  crystal_sequence -> add_talent( this, "Supernova",
                                  "if=mana.pct<96" );
  crystal_sequence -> add_action( this, "Presence of Mind",
                                  "if=cooldown.cold_snap.up|pet.prismatic_crystal.remains<2*spell_haste" );
  crystal_sequence -> add_action( this, "Arcane Blast",
                                  "if=buff.arcane_charge.stack=4&mana.pct>93&pet.prismatic_crystal.remains>cast_time" );
  crystal_sequence -> add_action( this, "Arcane Missiles",
                                  "if=pet.prismatic_crystal.remains>2*spell_haste+(target.distance%20)" );
  crystal_sequence -> add_talent( this, "Supernova",
                                  "if=pet.prismatic_crystal.remains<2*spell_haste+(target.distance%20)" );
  crystal_sequence -> add_action( "choose_target,if=pet.prismatic_crystal.remains<action.arcane_blast.cast_time&buff.presence_of_mind.down" );
  crystal_sequence -> add_action( this, "Arcane Blast" );


  cooldowns -> add_action( this, "Arcane Power",
                           "",
                           "Consolidated damage cooldown abilities" );
  for( size_t i = 0; i < racial_actions.size(); i++ )
  {
    cooldowns -> add_action( racial_actions[i] );
  }
  cooldowns -> add_action( get_potion_action() + ",if=buff.arcane_power.up&(!talent.prismatic_crystal.enabled|pet.prismatic_crystal.active)" );
  for( size_t i = 0; i < item_actions.size(); i++ )
  {
    cooldowns -> add_action( item_actions[i] );
  }


  aoe -> add_action( "call_action_list,name=cooldowns",
                     "AoE sequence" );
  aoe -> add_talent( this, "Nether Tempest",
                     "cycle_targets=1,if=buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<3.6))" );
  aoe -> add_talent( this, "Supernova" );
  aoe -> add_talent( this, "Arcane Orb",
                     "if=buff.arcane_charge.stack<4" );
  aoe -> add_action( this, "Arcane Explosion",
                     "if=prev_gcd.evocation",
                     "APL hack for evocation interrupt" );
  aoe -> add_action( this, "Evocation",
                     "interrupt_if=mana.pct>96,if=mana.pct<85-2.5*buff.arcane_charge.stack" );
  aoe -> add_action( this, "Arcane Missiles",
                     "if=set_bonus.tier17_4pc&active_enemies<10&buff.arcane_charge.stack=4&buff.arcane_instability.react" );
  aoe -> add_action( this, "Arcane Missiles",
                     "target_if=debuff.mark_of_doom.remains>2*spell_haste+(target.distance%20),if=buff.arcane_missiles.react" );
  aoe -> add_talent( this, "Nether Tempest",
                     "cycle_targets=1,if=talent.arcane_orb.enabled&buff.arcane_charge.stack=4&ticking&remains<cooldown.arcane_orb.remains" );
  aoe -> add_action( this, "Arcane Barrage",
                     "if=buff.arcane_charge.stack=4" );
  aoe -> add_action( this, "Cone of Cold",
                     "if=glyph.cone_of_cold.enabled" );
  aoe -> add_action( this, "Arcane Explosion" );


  burn -> add_action( "call_action_list,name=init_crystal,if=talent.prismatic_crystal.enabled&cooldown.prismatic_crystal.up",
                      "High mana usage, \"Burn\" sequence" );
  burn -> add_action( "call_action_list,name=crystal_sequence,if=talent.prismatic_crystal.enabled&pet.prismatic_crystal.active" );
  burn -> add_action( "call_action_list,name=cooldowns" );
  burn -> add_action( this, "Arcane Missiles",
                      "if=buff.arcane_missiles.react=3" );
  burn -> add_action( this, "Arcane Missiles",
                      "if=set_bonus.tier17_4pc&buff.arcane_instability.react&buff.arcane_instability.remains<action.arcane_blast.execute_time" );
  burn -> add_talent( this, "Supernova",
                      "if=target.time_to_die<8|charges=2" );
  burn -> add_talent( this, "Nether Tempest",
                      "cycle_targets=1,if=target!=pet.prismatic_crystal&buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<3.6))" );
  burn -> add_talent( this, "Arcane Orb",
                      "if=buff.arcane_charge.stack<4" );
  burn -> add_action( this, "Arcane Barrage",
                      "if=talent.arcane_orb.enabled&active_enemies>=3&buff.arcane_charge.stack=4&(cooldown.arcane_orb.remains<gcd.max|prev_gcd.arcane_orb)" );
  burn -> add_action( this, "Presence of Mind",
                      "if=mana.pct>96&(!talent.prismatic_crystal.enabled|!cooldown.prismatic_crystal.up)" );
  burn -> add_action( this, "Arcane Blast",
                      "if=buff.arcane_charge.stack=4&mana.pct>93" );
  burn -> add_action( this, "Arcane Missiles",
                      "if=buff.arcane_charge.stack=4&(mana.pct>70|!cooldown.evocation.up|target.time_to_die<15)" );
  burn -> add_talent( this, "Supernova",
                      "if=mana.pct>70&mana.pct<96" );
  burn -> add_action( this, "Evocation",
                      "interrupt_if=mana.pct>100-10%spell_haste,if=target.time_to_die>10&mana.pct<30+2.5*active_enemies*(9-active_enemies)-(40*(t18_class_trinket&buff.arcane_power.up))" );
  burn -> add_action( this, "Presence of Mind",
                      "if=!talent.prismatic_crystal.enabled|!cooldown.prismatic_crystal.up" );
  burn -> add_action( this, "Arcane Blast" );
  burn -> add_action( this, "Evocation" );


  conserve -> add_action( "call_action_list,name=cooldowns,if=target.time_to_die<15",
                          "Low mana usage, \"Conserve\" sequence" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=buff.arcane_missiles.react=3|(talent.overpowered.enabled&buff.arcane_power.up&buff.arcane_power.remains<action.arcane_blast.execute_time)" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=set_bonus.tier17_4pc&buff.arcane_instability.react&buff.arcane_instability.remains<action.arcane_blast.execute_time" );
  conserve -> add_talent( this, "Nether Tempest",
                          "cycle_targets=1,if=target!=pet.prismatic_crystal&buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<3.6))" );
  conserve -> add_talent( this, "Supernova",
                          "if=target.time_to_die<8|(charges=2&(buff.arcane_power.up|!cooldown.arcane_power.up|!legendary_ring.cooldown.up)&(!talent.prismatic_crystal.enabled|cooldown.prismatic_crystal.remains>8))" );
  conserve -> add_talent( this, "Arcane Orb",
                          "if=buff.arcane_charge.stack<2" );
  conserve -> add_action( this, "Presence of Mind",
                          "if=mana.pct>96&(!talent.prismatic_crystal.enabled|!cooldown.prismatic_crystal.up)" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=buff.arcane_missiles.react&debuff.mark_of_doom.remains>2*spell_haste+(target.distance%20)" );
  conserve -> add_action( this, "Arcane Blast",
                          "if=buff.arcane_charge.stack=4&mana.pct>93" );
  conserve -> add_action( this, "Arcane Barrage",
                          "if=talent.arcane_orb.enabled&active_enemies>=3&buff.arcane_charge.stack=4&(cooldown.arcane_orb.remains<gcd.max|prev_gcd.arcane_orb)" );
  conserve -> add_action( this, "Arcane Missiles",
                          "if=buff.arcane_charge.stack=4&(!talent.overpowered.enabled|cooldown.arcane_power.remains>10*spell_haste|legendary_ring.cooldown.remains>10*spell_haste)" );
  conserve -> add_talent( this, "Supernova",
                          "if=mana.pct<96&(buff.arcane_missiles.stack<2|buff.arcane_charge.stack=4)&(buff.arcane_power.up|(charges=1&(cooldown.arcane_power.remains>recharge_time|legendary_ring.cooldown.remains>recharge_time)))&(!talent.prismatic_crystal.enabled|current_target=pet.prismatic_crystal|(charges=1&cooldown.prismatic_crystal.remains>recharge_time+8))" );
  conserve -> add_talent( this, "Nether Tempest",
                          "cycle_targets=1,if=target!=pet.prismatic_crystal&buff.arcane_charge.stack=4&(active_dot.nether_tempest=0|(ticking&remains<(10-3*talent.arcane_orb.enabled)*spell_haste))" );
  conserve -> add_action( this, "Arcane Barrage",
                          "if=buff.arcane_charge.stack=4" );
  conserve -> add_action( this, "Presence of Mind",
                          "if=buff.arcane_charge.stack<2&mana.pct>93" );
  conserve -> add_action( this, "Arcane Blast" );
  conserve -> add_action( this, "Arcane Barrage" );
}

// Fire Mage Action List ===================================================================================================

void mage_t::apl_fire()
{
  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"           );

  action_priority_list_t* movement            = get_action_priority_list( "movement"         );
  action_priority_list_t* crystal_sequence    = get_action_priority_list( "crystal_sequence"  );
  action_priority_list_t* init_combust        = get_action_priority_list( "init_combust"      );
  action_priority_list_t* combust_sequence    = get_action_priority_list( "combust_sequence"  );
  action_priority_list_t* active_talents      = get_action_priority_list( "active_talents"    );
  action_priority_list_t* living_bomb         = get_action_priority_list( "living_bomb"       );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe"               );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target"     );


  default_list -> add_action( "stop_pyro_chain,if=prev_off_gcd.combustion" );
  default_list -> add_action( this, "Counterspell",
                              "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Time Warp",
                              "if=target.health.pct<25|time>5" );
  default_list -> add_action( "call_action_list,name=movement,if=raid_event.movement.exists" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<cast_time" );
  default_list -> add_action( "call_action_list,name=combust_sequence,if=pyro_chain" );
  default_list -> add_action( "call_action_list,name=crystal_sequence,if=talent.prismatic_crystal.enabled&pet.prismatic_crystal.active" );
  default_list -> add_action( "call_action_list,name=init_combust,if=!pyro_chain" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<action.fireball.execute_time+gcd.max&!(buff.heating_up.up&action.fireball.in_flight)",
                              "Utilize level 90 active talents while avoiding pyro munching" );
  default_list -> add_talent( this, "Mirror Image",
                              "if=!(buff.heating_up.up&action.fireball.in_flight)" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>10" );
  default_list -> add_action( "call_action_list,name=single_target");


  movement -> add_action( this, "Blink",
                          "if=movement.distance>10" );
  movement -> add_talent( this, "Blazing Speed",
                          "if=movement.remains>0" );
  movement -> add_talent( this, "Ice Floes",
                          "if=buff.ice_floes.down&(raid_event.movement.distance>0|raid_event.movement.in<action.fireball.cast_time)" );


  // TODO: Add multi LB explosions on multitarget fights.
  crystal_sequence -> add_action( "choose_target,name=prismatic_crystal",
                                  "Action list while Prismatic Crystal is up" );
  crystal_sequence -> add_action( this, "Inferno Blast",
                                  "if=dot.combustion.ticking&active_dot.combustion<active_enemies+1",
                                  "Spread Combustion from PC" );
  crystal_sequence -> add_action( this, "Inferno Blast",
                                  "cycle_targets=1,if=dot.combustion.ticking&active_dot.combustion<active_enemies",
                                  "Spread Combustion on multitarget fights" );
  crystal_sequence -> add_talent( this, "Cold Snap",
                                  "if=!cooldown.dragons_breath.up" );
  crystal_sequence -> add_action( this, "Dragon's Breath",
                                  "if=glyph.dragons_breath.enabled" );
  crystal_sequence -> add_talent( this, "Blast Wave" );
  crystal_sequence -> add_action( this, "Pyroblast",
                                  "if=execute_time=gcd.max&pet.prismatic_crystal.remains<gcd.max+travel_time&pet.prismatic_crystal.remains>travel_time",
                                  "Use pyros before PC's expiration" );
  crystal_sequence -> add_action( "call_action_list,name=single_target" );


  init_combust -> add_action( "start_pyro_chain,if=talent.meteor.enabled&cooldown.meteor.up&(legendary_ring.cooldown.remains<gcd.max|legendary_ring.cooldown.remains>target.time_to_die+15|!legendary_ring.has_cooldown)&((cooldown.combustion.remains<gcd.max*3&buff.pyroblast.up&(buff.heating_up.up^action.fireball.in_flight))|(buff.pyromaniac.up&(cooldown.combustion.remains<ceil(buff.pyromaniac.remains%gcd.max)*gcd.max)))",
                              "Combustion sequence initialization\n"
                              "# This sequence lists the requirements for preparing a Combustion combo with each talent choice\n"
                              "# Meteor Combustion" );
  init_combust -> add_action( "start_pyro_chain,if=talent.prismatic_crystal.enabled&cooldown.prismatic_crystal.up&(legendary_ring.cooldown.remains<gcd.max&(!equipped.112320|trinket.proc.crit.react)|legendary_ring.cooldown.remains+20>target.time_to_die|!legendary_ring.has_cooldown)&((cooldown.combustion.remains<gcd.max*2&buff.pyroblast.up&(buff.heating_up.up^action.fireball.in_flight))|(buff.pyromaniac.up&(cooldown.combustion.remains<ceil(buff.pyromaniac.remains%gcd.max)*gcd.max)))",
                              "Prismatic Crystal Combustion" );
  init_combust -> add_action( "start_pyro_chain,if=talent.prismatic_crystal.enabled&!glyph.combustion.enabled&cooldown.prismatic_crystal.remains>20&((cooldown.combustion.remains<gcd.max*2&buff.pyroblast.up&buff.heating_up.up&action.fireball.in_flight)|(buff.pyromaniac.up&(cooldown.combustion.remains<ceil(buff.pyromaniac.remains%gcd.max)*gcd.max)))",
                              "Unglyphed Combustions between Prismatic Crystals" );
  init_combust -> add_action( "start_pyro_chain,if=!talent.prismatic_crystal.enabled&!talent.meteor.enabled&((cooldown.combustion.remains<gcd.max*4&buff.pyroblast.up&buff.heating_up.up&action.fireball.in_flight)|(buff.pyromaniac.up&cooldown.combustion.remains<ceil(buff.pyromaniac.remains%gcd.max)*(gcd.max+talent.kindling.enabled)))",
                              "Kindling or Level 90 Combustion" );

  combust_sequence -> add_talent( this, "Prismatic Crystal", "",
                                  "Combustion Sequence" );
  for( size_t i = 0; i < racial_actions.size(); i++ )
    combust_sequence -> add_action( racial_actions[i] );
  for( size_t i = 0; i < item_actions.size(); i++ )
    combust_sequence -> add_action( item_actions[i] );
  combust_sequence -> add_action( get_potion_action() );
  combust_sequence -> add_talent( this, "Meteor",
                                  "if=active_enemies<=2" );
  combust_sequence -> add_action( this, "Pyroblast",
                                  "if=set_bonus.tier17_4pc&buff.pyromaniac.up" );
  combust_sequence -> add_action( this, "Inferno Blast",
                                  "if=set_bonus.tier16_4pc_caster&(buff.pyroblast.up^buff.heating_up.up)" );
  combust_sequence -> add_action( this, "Fireball",
                                  "if=!dot.ignite.ticking&!in_flight" );
  combust_sequence -> add_action( this, "Fireball",
                                  "if=crit_pct_current-1>(1000%13)&prev_gcd.pyroblast&buff.pyroblast.up&buff.heating_up.up&12-pet.prismatic_crystal.remains<action.fireball.execute_time+3*gcd.max&spell_haste<0.7" );
  combust_sequence -> add_action( this, "Pyroblast",
                                  "if=buff.pyroblast.up&dot.ignite.tick_dmg*(6-ceil(dot.ignite.remains-travel_time))<crit_damage*mastery_value" );
  combust_sequence -> add_action( this, "Inferno Blast",
                                  "if=talent.meteor.enabled&cooldown.meteor.duration-cooldown.meteor.remains<gcd.max*3",
                                  "Meteor Combustions can run out of Pyro procs before impact. Use IB to delay Combustion" );
  combust_sequence -> add_action( this, "Inferno Blast",
                                  "if=dot.ignite.tick_dmg*(6-dot.ignite.ticks_remain)<crit_damage*mastery_value" );
  combust_sequence -> add_action( this, "Combustion" );


  active_talents -> add_talent( this, "Meteor",
                                "if=active_enemies>=3|(glyph.combustion.enabled&(!talent.incanters_flow.enabled|buff.incanters_flow.stack+incanters_flow_dir>=4)&cooldown.meteor.duration-cooldown.combustion.remains<10)",
                                "Active talents usage" );
  active_talents -> add_action( "call_action_list,name=living_bomb,if=talent.living_bomb.enabled&(active_enemies>1|raid_event.adds.in<10)" );
  active_talents -> add_talent( this, "Cold Snap",
                                "if=glyph.dragons_breath.enabled&!talent.prismatic_crystal.enabled&!cooldown.dragons_breath.up" );
  active_talents -> add_action( this, "Dragon's Breath",
                                "if=glyph.dragons_breath.enabled&(!talent.prismatic_crystal.enabled|cooldown.prismatic_crystal.remains>8|legendary_ring.cooldown.remains>10)" );
  active_talents -> add_talent( this, "Blast Wave",
                                "if=(!talent.incanters_flow.enabled|buff.incanters_flow.stack>=4)&(target.time_to_die<10|!talent.prismatic_crystal.enabled|(charges>=1&cooldown.prismatic_crystal.remains>recharge_time))" );


  living_bomb -> add_action( this, "Inferno Blast",
                             "cycle_targets=1,if=dot.living_bomb.ticking&active_enemies-active_dot.living_bomb>1",
                             "Living Bomb application" );
  living_bomb -> add_talent( this, "Living Bomb",
                             "if=target!=pet.prismatic_crystal&(((!talent.incanters_flow.enabled|incanters_flow_dir<0|buff.incanters_flow.stack=5)&remains<3.6)|((incanters_flow_dir>0|buff.incanters_flow.stack=1)&remains<gcd.max))&target.time_to_die>remains+12" );


  aoe -> add_action( this, "Inferno Blast",
                     "cycle_targets=1,if=(dot.combustion.ticking&active_dot.combustion<active_enemies)|(dot.pyroblast.ticking&active_dot.pyroblast<active_enemies)",
                     "AoE sequence" );
  aoe -> add_action( "call_action_list,name=active_talents" );
  aoe -> add_action( this, "Pyroblast",
                     "if=buff.pyroblast.react|buff.pyromaniac.react" );
  aoe -> add_talent( this, "Cold Snap",
                     "if=!cooldown.dragons_breath.up" );
  aoe -> add_action( this, "Dragon's Breath" );
  aoe -> add_action( this, "Flamestrike",
                     "if=mana.pct>10&remains<2.4"  );


  single_target -> add_action( this, "Inferno Blast",
                               "if=dot.combustion.ticking&active_dot.combustion<active_enemies",
                               "Single target sequence" );
  single_target -> add_action( this, "Pyroblast",
                               "if=buff.pyroblast.up&buff.pyroblast.remains<action.fireball.execute_time",
                               "Use Pyro procs before they run out" );
  single_target -> add_action( this, "Pyroblast",
                               "if=set_bonus.tier16_2pc_caster&buff.pyroblast.up&buff.potent_flames.up&buff.potent_flames.remains<gcd.max" );
  single_target -> add_action( this, "Pyroblast",
                               "if=set_bonus.tier17_4pc&buff.pyromaniac.react" );
  single_target -> add_action( this, "Pyroblast",
                               "if=buff.pyroblast.up&buff.heating_up.up&action.fireball.in_flight",
                               "Pyro camp during regular sequence; Do not use Pyro procs without HU and first using fireball" );
  single_target -> add_action( this, "Inferno Blast",
                               "if=buff.pyroblast.down&buff.heating_up.up&!(dot.living_bomb.remains>10&active_enemies>1)",
                               "Heating Up conversion to Pyroblast" );
  single_target -> add_action( "call_action_list,name=active_talents" );
  single_target -> add_action( this, "Inferno Blast",
                               "if=buff.pyroblast.up&buff.heating_up.down&!action.fireball.in_flight&!(dot.living_bomb.remains>10&active_enemies>1)",
                               "Adding Heating Up to Pyroblast" );
  single_target -> add_action( this, "Fireball" );
  single_target -> add_action( this, "Scorch", "moving=1" );
}

// Frost Mage Action List ==============================================================================================================

void mage_t::apl_frost()
{
  std::vector<std::string> item_actions = get_item_actions();
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list      = get_action_priority_list( "default"          );

  action_priority_list_t* movement          = get_action_priority_list( "movement"         );
  action_priority_list_t* crystal_sequence  = get_action_priority_list( "crystal_sequence" );
  action_priority_list_t* cooldowns         = get_action_priority_list( "cooldowns"        );
  action_priority_list_t* init_water_jet    = get_action_priority_list( "init_water_jet"   );
  action_priority_list_t* water_jet         = get_action_priority_list( "water_jet"        );
  action_priority_list_t* aoe               = get_action_priority_list( "aoe"              );
  action_priority_list_t* single_target     = get_action_priority_list( "single_target"    );


  default_list -> add_action( this, "Counterspell",
                              "if=target.debuff.casting.react" );
  default_list -> add_action( this, "Time Warp",
                              "if=target.health.pct<25|time>5" );
  default_list -> add_action( "call_action_list,name=movement,if=raid_event.movement.exists" );
  default_list -> add_talent( this, "Mirror Image" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=buff.rune_of_power.remains<cast_time" );
  default_list -> add_talent( this, "Rune of Power",
                              "if=(cooldown.icy_veins.remains<gcd.max&buff.rune_of_power.remains<20)|(cooldown.prismatic_crystal.remains<gcd.max&buff.rune_of_power.remains<10)" );
  default_list -> add_action( "water_jet,if=time<1&active_enemies<4&!talent.prismatic_crystal.enabled",
                              "Water jet on pull for non PC talent combos" );
  default_list -> add_action( "call_action_list,name=cooldowns,if=target.time_to_die<24" );
  default_list -> add_action( "call_action_list,name=crystal_sequence,if=talent.prismatic_crystal.enabled&(cooldown.prismatic_crystal.remains<=gcd.max|pet.prismatic_crystal.active)&(legendary_ring.cooldown.remains<gcd.max|legendary_ring.cooldown.remains+15>target.time_to_die|!legendary_ring.has_cooldown)" );
  default_list -> add_action( "call_action_list,name=water_jet,if=prev_off_gcd.water_jet|debuff.water_jet.remains>0" );
  default_list -> add_action( "call_action_list,name=aoe,if=active_enemies>=4" );
  default_list -> add_action( "call_action_list,name=single_target" );


  movement -> add_action( this, "Blink",
                          "if=movement.distance>10" );
  movement -> add_talent( this, "Blazing Speed",
                          "if=movement.remains>0" );
  movement -> add_talent( this, "Ice Floes",
                          "if=buff.ice_floes.down&(raid_event.movement.distance>0|raid_event.movement.in<action.frostbolt.cast_time)" );


  crystal_sequence -> add_talent( this, "Frost Bomb",
                                  "if=active_enemies=1&current_target!=pet.prismatic_crystal&remains<10",
                                  "Actions while Prismatic Crystal is active" );
  crystal_sequence -> add_action( this, "Frozen Orb",
                                  "target_if=max:target.time_to_die&target!=pet.prismatic_crystal" );
  crystal_sequence -> add_talent( this, "Prismatic Crystal" );
  crystal_sequence -> add_action( "water_jet,if=pet.prismatic_crystal.remains>(5+10*set_bonus.tier18_4pc)*spell_haste*0.9" );
  crystal_sequence -> add_action( "call_action_list,name=cooldowns" );
  crystal_sequence -> add_talent( this, "Frost Bomb",
                                  "if=talent.prismatic_crystal.enabled&current_target=pet.prismatic_crystal&active_enemies>1&!ticking" );
  crystal_sequence -> add_action( this, "Ice Lance",
                                  "if=!t18_class_trinket&(buff.fingers_of_frost.react>=2+set_bonus.tier18_4pc*2|(buff.fingers_of_frost.react>set_bonus.tier18_4pc*2&active_dot.frozen_orb))" );
  crystal_sequence -> add_talent( this, "Ice Nova",
                                  "if=charges=2|pet.prismatic_crystal.remains<4" );
  crystal_sequence -> add_action( this, "Ice Lance",
                                  "if=buff.fingers_of_frost.react&buff.shatterlance.up" );
  crystal_sequence -> add_action( this, "Frostfire Bolt",
                                  "if=buff.brain_freeze.react=2" );
  crystal_sequence -> add_action( this, "Frostbolt",
                                  "target_if=max:debuff.water_jet.remains,if=t18_class_trinket&buff.fingers_of_frost.react&!buff.shatterlance.up&pet.prismatic_crystal.remains>cast_time" );
  crystal_sequence -> add_action( this, "Ice Lance",
                                  "if=buff.fingers_of_frost.react" );
  crystal_sequence -> add_action( this, "Frostfire Bolt",
                                  "if=buff.brain_freeze.react" );
  crystal_sequence -> add_talent( this, "Ice Nova" );
  crystal_sequence -> add_action( this, "Blizzard",
                                 "interrupt_if=cooldown.frozen_orb.up|(talent.frost_bomb.enabled&buff.fingers_of_frost.react>=2+set_bonus.tier18_4pc),if=active_enemies>=5" );
  crystal_sequence -> add_action( "choose_target,if=pet.prismatic_crystal.remains<action.frostbolt.cast_time+action.frostbolt.travel_time" );
  crystal_sequence -> add_action( this, "Frostbolt" );


  cooldowns -> add_action( this, "Icy Veins",
                           "",
                           "Consolidated damage cooldown abilities" );

  for( size_t i = 0; i < racial_actions.size(); i++ )
    cooldowns -> add_action( racial_actions[i] );

  cooldowns -> add_action( get_potion_action() + ",if=buff.bloodlust.up|buff.icy_veins.up" );

  for( size_t i = 0; i < item_actions.size(); i++ )
    cooldowns -> add_action( item_actions[i] );


  init_water_jet -> add_talent( this, "Frost Bomb",
                                "if=remains<4*spell_haste*(1+set_bonus.tier18_4pc)+cast_time",
                                "Water Jet initialization" );
  init_water_jet -> add_action( "water_jet,if=prev_gcd.frostbolt|action.frostbolt.travel_time<spell_haste" );
  init_water_jet -> add_action( this, "Frostbolt" );


  water_jet -> add_action( this, "Frostbolt",
                           "if=prev_off_gcd.water_jet",
                           "Water Jet sequence" );
  water_jet -> add_action( this, "Ice Lance",
                           "if=set_bonus.tier18_4pc&buff.fingers_of_frost.react>2*set_bonus.tier18_4pc&buff.shatterlance.up" );
  water_jet -> add_action( this, "Frostfire Bolt",
                           "if=set_bonus.tier18_4pc&buff.brain_freeze.react=2" );
  water_jet -> add_action( this, "frostbolt",
                           "if=t18_class_trinket&debuff.water_jet.remains>cast_time+travel_time&buff.fingers_of_frost.react&!buff.shatterlance.up" );
  water_jet -> add_action( this, "Ice Lance",
                           "if=!t18_class_trinket&buff.fingers_of_frost.react>=2+2*set_bonus.tier18_4pc&action.frostbolt.in_flight" );
  water_jet -> add_action( this, "Frostbolt",
                           "if=!set_bonus.tier18_4pc&debuff.water_jet.remains>cast_time+travel_time" );


  aoe -> add_action( "call_action_list,name=cooldowns",
                     "AoE sequence" );
  aoe -> add_talent( this, "Frost Bomb",
                     "if=remains<action.ice_lance.travel_time&(cooldown.frozen_orb.remains<gcd.max|buff.fingers_of_frost.react>=2)" );
  aoe -> add_action( this, "Frozen Orb" );
  aoe -> add_action( this, "Ice Lance",
                     "if=talent.frost_bomb.enabled&buff.fingers_of_frost.react&debuff.frost_bomb.up" );
  aoe -> add_talent( this, "Comet Storm" );
  aoe -> add_talent( this, "Ice Nova" );
  aoe -> add_action( this, "Blizzard",
                     "interrupt_if=cooldown.frozen_orb.up|(talent.frost_bomb.enabled&buff.fingers_of_frost.react>=2)" );


  single_target -> add_action( "call_action_list,name=cooldowns,if=!talent.prismatic_crystal.enabled|(cooldown.prismatic_crystal.remains>15&!legendary_ring.has_cooldown)",
                               "Single target sequence" );
  single_target -> add_action( this, "Ice Lance",
                               "if=buff.fingers_of_frost.react&(buff.fingers_of_frost.remains<action.frostbolt.execute_time|buff.fingers_of_frost.remains<buff.fingers_of_frost.react*gcd.max)",
                               "Safeguards against losing FoF and BF to buff expiry" );
  single_target -> add_action( this, "Frostfire Bolt",
                               "if=buff.brain_freeze.react&(buff.brain_freeze.remains<action.frostbolt.execute_time|buff.brain_freeze.remains<buff.brain_freeze.react*gcd.max)" );
  single_target -> add_talent( this, "Frost Bomb",
                               "if=(!talent.prismatic_crystal.enabled|legendary_ring.cooldown.remains>45)&cooldown.frozen_orb.remains<gcd.max&debuff.frost_bomb.remains<10",
                               "Frozen Orb usage without Prismatic Crystal" );
  single_target -> add_action( this, "Frozen Orb",
                               "if=(!talent.prismatic_crystal.enabled|legendary_ring.cooldown.remains>45)&buff.fingers_of_frost.stack<2&cooldown.icy_veins.remains>45-20*talent.thermal_void.enabled" );
  single_target -> add_action( this, "Ice Lance",
                               "if=buff.fingers_of_frost.react&buff.shatterlance.up",
                               "Single target routine; Rough summary: IN2 > FoF2 > CmS > IN > BF > FoF" );
  single_target -> add_talent( this, "Frost Bomb",
                               "if=remains<action.ice_lance.travel_time+t18_class_trinket*action.frostbolt.execute_time&(buff.fingers_of_frost.react>=(2+set_bonus.tier18_4pc*2)%(1+t18_class_trinket)|(buff.fingers_of_frost.react&(talent.thermal_void.enabled|buff.fingers_of_frost.remains<gcd.max*(buff.fingers_of_frost.react+1))))" );
  single_target -> add_talent( this, "Ice Nova",
                               "if=target.time_to_die<10|(charges=2&(!talent.prismatic_crystal.enabled|!cooldown.prismatic_crystal.up))" );
  single_target -> add_action( this, "Ice Lance",
                               "if=!t18_class_trinket&(buff.fingers_of_frost.react>=2+set_bonus.tier18_4pc*2|(buff.fingers_of_frost.react>1+set_bonus.tier18_4pc&active_dot.frozen_orb))" );
  single_target -> add_talent( this, "Comet Storm" );
  single_target -> add_talent( this, "Ice Nova",
                               "if=(!talent.prismatic_crystal.enabled|(charges=1&cooldown.prismatic_crystal.remains>recharge_time&(buff.incanters_flow.stack>3|!talent.ice_nova.enabled)))&(buff.icy_veins.up|(charges=1&cooldown.icy_veins.remains>recharge_time))" );
  single_target -> add_action( this, "Frostfire Bolt",
                               "if=buff.brain_freeze.react" );
  single_target -> add_action( "call_action_list,name=init_water_jet,if=pet.water_elemental.cooldown.water_jet.remains<=gcd.max*talent.frost_bomb.enabled&buff.fingers_of_frost.react<2+2*set_bonus.tier18_4pc&!active_dot.frozen_orb" );
  single_target -> add_action( this, "Frostbolt",
                               "if=t18_class_trinket&buff.fingers_of_frost.react&!buff.shatterlance.up" );
  single_target -> add_action( this, "Ice Lance",
                               "if=talent.frost_bomb.enabled&buff.fingers_of_frost.react&debuff.frost_bomb.remains>travel_time&(!talent.thermal_void.enabled|cooldown.icy_veins.remains>8)" );
  single_target -> add_action( this, "Frostbolt",
                               "if=set_bonus.tier17_2pc&buff.ice_shard.up&!(talent.thermal_void.enabled&buff.icy_veins.up&buff.icy_veins.remains<10)",
                               "Camp procs and spam Frostbolt while 4T17 buff is up" );
  single_target -> add_action( this, "Ice Lance",
                               "if=!talent.frost_bomb.enabled&buff.fingers_of_frost.react&(!talent.thermal_void.enabled|cooldown.icy_veins.remains>8)" );
  single_target -> add_action( this, "Frostbolt" );
  single_target -> add_action( this, "Ice Lance", "moving=1" );

}

// Default Action List ===============================================================================================

void mage_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( "Frostfire Bolt" );
}

// mage_t::mana_regen_per_second ============================================

double mage_t::mana_regen_per_second() const
{
  double mps = player_t::mana_regen_per_second();

  if ( passives.nether_attunement -> ok() )
    mps /= cache.spell_haste();

  return mps;
}

double mage_t::composite_rating_multiplier( rating_e rating) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
  case RATING_MULTISTRIKE:
    return m *= 1.0 + spec.ice_shards -> effectN( 1 ).percent();
  case RATING_SPELL_CRIT:
    return m *= 1.0 + spec.incineration -> effectN( 1 ).percent();
  case RATING_MASTERY:
    return m *= 1.0 + spec.arcane_mind -> effectN( 1 ).percent();
  default:
    break;
  }

  return m;
}

// mage_t::composite_player_multiplier =======================================

double mage_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( specialization() == MAGE_ARCANE )
  {
    if ( buffs.arcane_power -> up() )
    {
      double v = buffs.arcane_power -> value();
      if ( sets.has_set_bonus( SET_CASTER, T14, B4 ) )
      {
        v += 0.1;
      }
      m *= 1.0 + v;
    }

    cache.player_mult_valid[ school ] = false;
  }

  if ( talents.rune_of_power -> ok() )
  {
    if ( buffs.rune_of_power -> check() )
    {
      m *= 1.0 + buffs.rune_of_power -> data().effectN( 1 ).percent();
    }
  }
  else if ( talents.incanters_flow -> ok() )
  {
    m *= 1.0 + buffs.incanters_flow -> stack() * incanters_flow_stack_mult;

    cache.player_mult_valid[ school ] = false;
  }

  if ( buffs.icarus_uprising -> check() )
  {
    m *= 1.0 + buffs.icarus_uprising -> data().effectN( 2 ).percent();
  }

  if ( buffs.temporal_power -> check() &&
       sets.has_set_bonus( MAGE_ARCANE, T18, B4 ) )
  {
    m *= std::pow( 1.0 + buffs.temporal_power -> data().effectN( 1 ).percent(),
                   buffs.temporal_power -> check() );
  }

  return m;
}


void mage_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( spec.mana_adept -> ok() )
      {
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      }
      break;
    default: break;
  }
}

// mage_t::composite_spell_crit =============================================

double mage_t::composite_spell_crit() const
{
  double c = player_t::composite_spell_crit();

  if ( buffs.molten_armor -> up() )
  {
    c += buffs.molten_armor -> data().effectN( 1 ).percent();
  }

  return c;
}

//mage_t::composite_multistrike =============================================

double mage_t::composite_multistrike() const
{
  double ms = player_t::composite_multistrike();

  if ( buffs.frost_armor -> up() )
    ms += buffs.frost_armor -> data().effectN( 1 ).percent();

  if ( buffs.icy_veins -> up() && glyphs.icy_veins -> ok() )
  {
    ms += glyphs.icy_veins -> effectN( 1 ).percent();

    if ( perks.improved_icy_veins -> ok() )
    {
      ms += perks.improved_icy_veins -> effectN( 2 ).percent();
    }
  }

  return ms;
}

// mage_t::composite_spell_haste ============================================

double mage_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.icy_veins -> up() && !glyphs.icy_veins -> ok() )
  {
    h *= iv_haste;
  }

  if ( buffs.icarus_uprising -> check() )
    h /= 1.0 + buffs.icarus_uprising -> data().effectN( 1 ).percent();

  return h;
}

// mage_t::matching_gear_multiplier =========================================

double mage_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// mage_t::reset ============================================================

void mage_t::reset()
{
  player_t::reset();

  current_target = target;

  icicles.clear();
  event_t::cancel( icicle_event );
  last_bomb_target = nullptr;

  burn_phase.reset();
  pyro_chain.reset();

  rppm_pyromaniac.reset();
  rppm_arcane_instability.reset();

  if ( sets.has_set_bonus( MAGE_ARCANE, T18, B2 ) )
  {
    pets::temporal_hero_t::randomize_last_summoned( this );
  }
}

// mage_t::stun =============================================================

void mage_t::stun()
{
  // FIX ME: override this to handle Blink
  player_t::stun();
}

// mage_t::update_movement==================================================

void mage_t::update_movement( timespan_t duration )
{
  player_t::update_movement( duration );

  double yards = duration.total_seconds() * composite_movement_speed();
  distance_from_rune += yards;

  if ( buffs.rune_of_power -> check() )
  {
    if ( distance_from_rune > talents.rune_of_power -> effectN( 2 ).radius() )
    {
      buffs.rune_of_power -> expire();
      if ( sim -> debug ) sim -> out_debug.printf( "%s lost Rune of Power due to moving more than 8 yards away from it.", name() );
    }
  }
}

// mage_t::temporary_movement_modifier ==================================

double mage_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  if ( buffs.blazing_speed -> up() )
    temporary = std::max( buffs.blazing_speed -> default_value, temporary );

  if ( buffs.improved_blink -> up() )
    temporary = std::max( buffs.improved_blink -> default_value, temporary );

  if ( buffs.improved_scorch -> up() )
    temporary = std::max( buffs.improved_scorch -> default_value, temporary );

  return temporary;
}

// mage_t::arise ============================================================

void mage_t::arise()
{
  player_t::arise();

  if ( talents.incanters_flow -> ok() )
    buffs.incanters_flow -> trigger();

  if ( passives.molten_armor -> ok() )
    buffs.molten_armor -> trigger();
  else if ( passives.frost_armor -> ok() )
    buffs.frost_armor -> trigger();
  else if ( passives.mage_armor -> ok() )
    buffs.mage_armor -> trigger();
}

// Copypasta, execept for target selection. This is a massive kludge. Buyer
// beware!

action_t* mage_t::select_action( const action_priority_list_t& list )
{
  player_t* action_target = nullptr;

  // Mark this action list as visited with the APL internal id
  visited_apls_ |= list.internal_id_mask;

  // Cached copy for recursion, we'll need it if we come back from a
  // call_action_list tree, with nothing to show for it.
  uint64_t _visited = visited_apls_;

  for (auto a : list.foreground_action_list)
  {
    visited_apls_ = _visited;
    

    if ( a -> background ) continue;

    if ( a -> wait_on_ready == 1 )
      break;

    // Change the target of the action before ready call ...
    if ( a -> target != current_target )
    {
      action_target = a -> target;
      a -> target = current_target;
    }

    if ( a -> ready() )
    {
      if ( a -> type != ACTION_CALL )
        return a;
      // Call_action_list action, don't execute anything, but rather recurse
      // into the called action list.
      else
      {
        call_action_list_t* call = static_cast<call_action_list_t*>( a );
        // Restore original target before recursing into the called action list
        if ( action_target )
        {
          a -> target = action_target;
          action_target = nullptr;
        }

        // If the called APLs bitflag (based on internal id) is up, we're in an
        // infinite loop, and need to cancel the sim
        if ( visited_apls_ & call -> alist -> internal_id_mask )
        {
          sim -> errorf( "%s action list in infinite loop", name() );
          sim -> cancel_iteration();
          sim -> cancel();
          return nullptr;
        }

        // We get an action from the call, return it
        if ( action_t* real_a = select_action( *call -> alist ) )
        {
          if ( real_a -> action_list )
            real_a -> action_list -> used = true;
          return real_a;
        }
      }
    }
    // Action not ready, restore target for extra safety
    else if ( action_target )
    {
      a -> target = action_target;
      action_target = nullptr;
    }
  }

  return nullptr;
}
// mage_t::create_expression ================================================

expr_t* mage_t::create_expression( action_t* a, const std::string& name_str )
{
  struct mage_expr_t : public expr_t
  {
    mage_t& mage;
    mage_expr_t( const std::string& n, mage_t& m ) :
      expr_t( n ), mage( m ) {}
  };

  // Current target expression support
  // "current_target" returns the actor id of the mage's current target
  // "current_target.<some other expression>" evaluates <some other expression> on the current
  // target of the mage
  if ( util::str_in_str_ci( name_str, "current_target" ) )
  {
    std::string::size_type offset = name_str.find( '.' );
    if ( offset != std::string::npos )
    {
      struct current_target_wrapper_expr_t : public target_wrapper_expr_t
      {
        mage_t& mage;

        current_target_wrapper_expr_t( action_t& action, const std::string& suffix_expr_str ) :
          target_wrapper_expr_t( action, "current_target_wrapper_expr_t", suffix_expr_str ),
          mage( static_cast<mage_t&>( *action.player ) )
        { }

        player_t* target() const override
        { assert( mage.current_target ); return mage.current_target; }
      };

      return new current_target_wrapper_expr_t( *a, name_str.substr( offset + 1 ) );
    }
    else
    {
      struct current_target_expr_t : public mage_expr_t
      {
        current_target_expr_t( const std::string& n, mage_t& m ) :
          mage_expr_t( n, m )
        { }

        double evaluate() override
        {
          assert( mage.current_target );
          return static_cast<double>( mage.current_target -> actor_index );
        }
      };

      return new current_target_expr_t( name_str, *this );
    }
  }

  // Default target expression support
  // "default_target" returns the actor id of the mage's default target (typically the first enemy
  // in the sim, e.g. fluffy_pillow)
  // "default_target.<some other expression>" evaluates <some other expression> on the default
  // target of the mage
  if ( util::str_compare_ci( name_str, "default_target" ) )
  {
    std::string::size_type offset = name_str.find( '.' );
    if ( offset != std::string::npos )
    {
      return target -> create_expression( a, name_str.substr( offset + 1 ) );
    }
    else
    {
      return make_ref_expr( name_str, target -> actor_index );
    }
  }

  // Incanters flow direction
  // Evaluates to:  0.0 if IF talent not chosen or IF stack unchanged
  //                1.0 if next IF stack increases
  //               -1.0 if IF stack decreases
  if ( name_str == "incanters_flow_dir" )
  {
    struct incanters_flow_dir_expr_t : public mage_expr_t
    {
      mage_t * mage;

      incanters_flow_dir_expr_t( mage_t& m ) :
        mage_expr_t( "incanters_flow_dir", m ), mage( &m )
      {}

      virtual double evaluate() override
      {
        if ( !mage -> talents.incanters_flow -> ok() )
          return 0.0;

        buff_t*flow = mage -> buffs.incanters_flow;
        if ( flow -> reverse )
          return flow -> current_stack == 1 ? 0.0: -1.0;
        else
          return flow -> current_stack == 5 ? 0.0: 1.0;
      }
    };

    return new incanters_flow_dir_expr_t( * this );
  }

  // Pyro Chain Flag Expression ===============================================
  if ( name_str == "pyro_chain" )
  {
    struct pyro_chain_expr_t : public mage_expr_t
    {
      pyro_chain_expr_t( mage_t& m ) :
        mage_expr_t( "pyro_chain", m )
      {}
      virtual double evaluate() override
      {
        return mage.pyro_chain.on();
      }
    };

    return new pyro_chain_expr_t( *this );
  }

  if ( name_str == "pyro_chain_duration" )
  {
    struct pyro_chain_duration_expr_t : public mage_expr_t
    {
      pyro_chain_duration_expr_t( mage_t& m ) :
        mage_expr_t( "pyro_chain_duration", m )
      {}
      virtual double evaluate() override
      {
        return mage.pyro_chain.duration( mage.sim -> current_time() )
                              .total_seconds();
      }
    };

    return new pyro_chain_duration_expr_t( *this );
  }

  // Arcane Burn Flag Expression ==============================================
  if ( name_str == "burn_phase" )
  {
    struct burn_phase_expr_t : public mage_expr_t
    {
      burn_phase_expr_t( mage_t& m ) :
        mage_expr_t( "burn_phase", m )
      {}
      virtual double evaluate() override
      {
        return mage.burn_phase.on();
      }
    };

    return new burn_phase_expr_t( *this );
  }

  if ( name_str == "burn_phase_duration" )
  {
    struct burn_phase_duration_expr_t : public mage_expr_t
    {
      burn_phase_duration_expr_t( mage_t& m ) :
        mage_expr_t( "burn_phase_duration", m )
      {}
      virtual double evaluate() override
      {
        return mage.burn_phase.duration( mage.sim -> current_time() )
                              .total_seconds();
      }
    };

    return new burn_phase_duration_expr_t( *this );
  }

  // Icicle Expressions =======================================================
  if ( util::str_compare_ci( name_str, "shooting_icicles" ) )
  {
    struct sicicles_expr_t : public mage_expr_t
    {
      sicicles_expr_t( mage_t& m ) : mage_expr_t( "shooting_icicles", m )
      { }
      double evaluate() override
      { return mage.icicle_event != nullptr; }
    };

    return new sicicles_expr_t( *this );
  }

  if ( util::str_compare_ci( name_str, "icicles" ) )
  {
    struct icicles_expr_t : public mage_expr_t
    {
      icicles_expr_t( mage_t& m ) : mage_expr_t( "icicles", m )
      { }

      double evaluate() override
      {
        if ( mage.icicles.size() == 0 )
          return 0;
        else if ( mage.sim -> current_time() - mage.icicles[ 0 ].first < mage.spec.icicles_driver -> duration() )
          return static_cast<double>(mage.icicles.size());
        else
        {
          size_t icicles = 0;
          for ( int i = as<int>( mage.icicles.size() - 1 ); i >= 0; i-- )
          {
            if ( mage.sim -> current_time() - mage.icicles[ i ].first >= mage.spec.icicles_driver -> duration() )
              break;

            icicles++;
          }

          return static_cast<double>(icicles);
        }
      }
    };

    return new icicles_expr_t( *this );
  }

  return player_t::create_expression( a, name_str );
}

// mage_t::convert_hybrid_stat ==============================================

stat_e mage_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  // This is all a guess at how the hybrid primaries will work, since they
  // don't actually appear on cloth gear yet. TODO: confirm behavior
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

icicle_data_t mage_t::get_icicle_object()
{
  if ( icicles.size() == 0 )
    return icicle_data_t( (double) 0, (stats_t*) nullptr );

  timespan_t threshold = spec.icicles_driver -> duration();

  auto idx = icicles.begin(),
                                         end = icicles.end();
  for ( ; idx < end; ++idx )
  {
    if ( sim -> current_time() - ( *idx ).first < threshold )
      break;
  }

  // Set of icicles timed out
  if ( idx != icicles.begin() )
    icicles.erase( icicles.begin(), idx );

  if ( icicles.size() > 0 )
  {
    icicle_data_t d = icicles.front().second;
    icicles.erase( icicles.begin() );
    return d;
  }

  return icicle_data_t( (double) 0, (stats_t*) nullptr );
}

void mage_t::trigger_icicle( const action_state_t* trigger_state, bool chain, player_t* chain_target )
{
  if ( ! spec.icicles -> ok() )
    return;

  if (icicles.size() == 0 )
    return;

  std::pair<double, stats_t*> d;

  player_t* icicle_target;
  if ( chain_target )
  {
    icicle_target = chain_target;
  }
  else
  {
    icicle_target = trigger_state -> target;
  }

  if ( chain && ! icicle_event )
  {
    d = get_icicle_object();
    if ( d.first == 0 )
      return;

    icicle_event = new ( *sim ) events::icicle_event_t( *this, d, icicle_target, true );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s icicle use on %s%s, damage=%f, total=%u",
                             name(), icicle_target -> name(),
                             chain ? " (chained)" : "", d.first,
                             as<unsigned>( icicles.size() ) );
  }
  else if ( ! chain )
  {
    d = get_icicle_object();
    if ( d.first == 0 )
      return;

    icicle -> base_dd_min = icicle -> base_dd_max = d.first;

    actions::icicle_state_t* new_state = debug_cast<actions::icicle_state_t*>( icicle -> get_state() );
    new_state -> target = icicle_target;
    new_state -> source = d.second;

    // Immediately execute icicles so the correct damage is carried into the travelling icicle
    // object.
    icicle -> pre_execute_state = new_state;
    icicle -> execute();

    if ( sim -> debug )
      sim -> out_debug.printf( "%s icicle use on %s%s, damage=%f, total=%u",
                             name(), icicle_target -> name(),
                             chain ? " (chained)" : "", d.first,
                             as<unsigned>( icicles.size() ) );
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class mage_report_t : public player_report_extension_t
{
public:
  mage_report_t( mage_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void) p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  mage_t& p;
};

// MAGE MODULE INTERFACE ====================================================

static void do_trinket_init( mage_t*                  p,
                             specialization_e         spec,
                             const special_effect_t*& ptr,
                             const special_effect_t&  effect )
{
  if ( !p -> find_spell( effect.spell_id ) -> ok() ||
       p -> specialization() != spec )
  {
    return;
  }

  ptr = &( effect );
}

static void wild_arcanist( special_effect_t& effect )
{
  mage_t* p = debug_cast<mage_t*>( effect.player );
  do_trinket_init( p, MAGE_ARCANE, p -> wild_arcanist, effect );
}

static void pyrosurge( special_effect_t& effect )
{
  mage_t* p = debug_cast<mage_t*>( effect.player );
  do_trinket_init( p, MAGE_FIRE, p -> pyrosurge, effect );
}

static void shatterlance( special_effect_t& effect )
{
  mage_t* p = debug_cast<mage_t*>( effect.player );
  do_trinket_init( p, MAGE_FROST, p -> shatterlance, effect );
}

struct mage_module_t : public module_t
{
  mage_module_t() : module_t( MAGE ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new mage_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new mage_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184903, wild_arcanist );
    unique_gear::register_special_effect( 184904, pyrosurge     );
    unique_gear::register_special_effect( 184905, shatterlance  );
  }

  virtual void register_hotfixes() const override
  {
    hotfix::register_effect( "Mage", "2015-07-20",
                              "Flamestrike impact damage increased by 50% ",
                              126904 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.50 )
      .verification_value( 0.77700 );

    hotfix::register_effect( "Mage", "2015-07-20",
                              "Flamestrike DOT damage increased by 50% ",
                              630 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.50 )
      .verification_value( 0.10125 );

    hotfix::register_effect( "Mage", "2015-07-20",
                              "Dragon's Breath damage increased by 150% ",
                              21276 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 2.50 )
      .verification_value( 0.86000 );


  }

  virtual bool valid() const override { return true; }
  virtual void init        ( player_t* ) const override {}
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end  ( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::mage()
{
  static mage_module_t m;
  return &m;
}

