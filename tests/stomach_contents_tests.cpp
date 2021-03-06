#include <stdio.h>
#include <memory>

#include "catch/catch.hpp"
#include "calendar.h"
#include "game.h"
#include "player.h"
#include "player_helpers.h"
#include "item.h"
#include "stomach.h"
#include "units.h"
#include "pldata.h"

void reset_time()
{
    calendar::turn = calendar( 0 );
    player &p = g->u;
    p.set_stored_kcal( p.get_healthy_kcal() );
    p.set_hunger( 0 );
    clear_player();
}

void pass_time( player &p, time_duration amt )
{
    for( auto turns = 1_turns; turns < amt; turns += 1_turns ) {
        calendar::turn.increment();
        p.update_body();
    }
}

void clear_stomach( player &p )
{
    p.guts.set_calories( 0 );
    p.stomach.set_calories( 0 );
    p.stomach.bowel_movement();
    p.guts.bowel_movement();
}

void set_all_vitamins( int target, player &p )
{
    p.vitamin_set( vitamin_id( "vitA" ), target );
    p.vitamin_set( vitamin_id( "vitB" ), target );
    p.vitamin_set( vitamin_id( "vitC" ), target );
    p.vitamin_set( vitamin_id( "iron" ), target );
    p.vitamin_set( vitamin_id( "calcium" ), target );
}

// time (in minutes) it takes for the player to feel hungry
// passes time on the calendar
time_duration time_until_hungry( player &p )
{
    unsigned int thirty_minutes = 0;
    do {
        pass_time( p, 30_minutes );
        thirty_minutes++;
    } while( p.get_hunger() < 40 ); // hungry
    return thirty_minutes * 30_minutes;
}

void print_stomach_contents( player &p, const bool print )
{
    if( !print ) {
        return;
    }
    printf( "stomach: %d/%d guts: %d/%d player: %d/%d hunger: %d\n", p.stomach.get_calories(),
            p.stomach.get_calories_absorbed(), p.guts.get_calories(),
            p.guts.get_calories_absorbed(), p.get_stored_kcal(), p.get_healthy_kcal(), p.get_hunger() );
    printf( "stomach: %d mL/ %d mL guts %d mL/ %d mL\n",
            units::to_milliliter<int>( p.stomach.contains() ),
            units::to_milliliter<int>( p.stomach.capacity() ),
            units::to_milliliter<int>( p.guts.contains() ),
            units::to_milliliter<int>( p.guts.capacity() ) );
    printf( "metabolic rate: %.2f\n", p.metabolic_rate() );
}

// this represents an amount of food you can eat to keep you fed for an entire day
// accounting for appropriate vitamins
void eat_all_nutrients( player &p )
{
    item f( "pizza_veggy" );
    p.eat( f );
    f = item( "pizza_veggy" );
    p.eat( f );
    f = item( "pizza_veggy" );
    p.eat( f );
    f = item( "pizza_veggy" );
    p.eat( f );
    f = item( "fried_livers" );
    p.eat( f );
    f = item( "chips3" );
    p.eat( f );
    f = item( "chips3" );
    p.eat( f );
    f = item( "chips3" );
    p.eat( f );
    f = item( "chips3" );
    p.eat( f );
}

// how long does it take to starve to death
// player does not thirst or tire or require vitamins
TEST_CASE( "starve_test" )
{
    // change this bool when editing the test
    const bool print_tests = false;
    player &dummy = g->u;
    reset_time();
    clear_stomach( dummy );
    if( print_tests ) {
        printf( "\n\n" );
    }
    unsigned int day = 0;
    do {
        if( print_tests ) {
            printf( "day %d: %d\n", day, dummy.get_stored_kcal() );
        }
        pass_time( dummy, 1_days );
        dummy.set_thirst( 0 );
        dummy.set_fatigue( 0 );
        set_all_vitamins( 0, dummy );
        day++;
    } while( dummy.get_stored_kcal() > 0 );
    if( print_tests ) {
        printf( "\n\n" );

        CHECK( day == 46 );
    }
}

// how long does it take to starve to death with extreme metabolism
// player does not thirst or tire or require vitamins
TEST_CASE( "starve_test_hunger3" )
{
    // change this bool when editing the test
    const bool print_tests = false;
    player &dummy = g->u;
    reset_time();
    clear_stomach( dummy );
    while( !( dummy.has_trait( trait_id( "HUNGER3" ) ) ) ) {
        dummy.mutate_towards( trait_id( "HUNGER3" ) );
    }
    clear_stomach( dummy );
    if( print_tests ) {
        printf( "\n\n" );
    }
    unsigned int day = 0;
    do {
        if( print_tests ) {
            printf( "day %d: %d\n", day, dummy.get_stored_kcal() );
        }
        pass_time( dummy, 1_days );
        dummy.set_thirst( 0 );
        dummy.set_fatigue( 0 );
        set_all_vitamins( 0, dummy );
        day++;
    } while( dummy.get_stored_kcal() > 0 );
    if( print_tests ) {
        printf( "\n\n" );
    }
    CHECK( day <= 15 );
    CHECK( day >= 14 );
}

// does eating enough food per day keep you alive
TEST_CASE( "all_nutrition_starve_test" )
{
    // change this bool when editing the test
    const bool print_tests = false;
    player &dummy = g->u;
    reset_time();
    clear_stomach( dummy );
    eat_all_nutrients( dummy );
    if( print_tests ) {
        printf( "\n\n" );
    }

    for( unsigned int day = 0; day <= 10; day++ ) {
        if( print_tests ) {
            printf( "day %d: %d\n", day, dummy.get_stored_kcal() );
        }
        pass_time( dummy, 1_days );
        dummy.set_thirst( 0 );
        dummy.set_fatigue( 0 );
        eat_all_nutrients( dummy );
        print_stomach_contents( dummy, print_tests );
    }
    if( print_tests ) {
        printf( "vitamins: vitA %d vitB %d vitC %d calcium %d iron %d\n",
                dummy.vitamin_get( vitamin_id( "vitA" ) ), dummy.vitamin_get( vitamin_id( "vitB" ) ),
                dummy.vitamin_get( vitamin_id( "vitC" ) ), dummy.vitamin_get( vitamin_id( "calcium" ) ),
                dummy.vitamin_get( vitamin_id( "iron" ) ) );
        printf( "\n\n" );
    }
    CHECK( dummy.get_stored_kcal() >= dummy.get_healthy_kcal() );
    // since vitamins drain very quickly, it is almost impossible to remain at 0
    CHECK( dummy.vitamin_get( vitamin_id( "vitA" ) ) >= -2 );
    CHECK( dummy.vitamin_get( vitamin_id( "vitB" ) ) >= -2 );
    CHECK( dummy.vitamin_get( vitamin_id( "vitC" ) ) >= -2 );
    CHECK( dummy.vitamin_get( vitamin_id( "iron" ) ) >= -2 );
    CHECK( dummy.vitamin_get( vitamin_id( "calcium" ) ) >= -2 );
}

// reasonable length of time to pass before hunger sets in
TEST_CASE( "hunger" )
{
    // change this bool when editing the test
    const bool print_tests = false;
    player &dummy = g->u;
    reset_time();
    clear_stomach( dummy );
    dummy.initialize_stomach_contents();

    if( print_tests ) {
        printf( "\n\n" );
    }
    print_stomach_contents( dummy, print_tests );
    int hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
        print_stomach_contents( dummy, print_tests );
        printf( "eat 2 cooked meat\n" );
    }
    CHECK( hunger_time <= 270 );
    CHECK( hunger_time >= 240 );
    item f( "meat_cooked" );
    dummy.eat( f );
    f = item( "meat_cooked" );
    dummy.eat( f );
    dummy.set_thirst( 0 );
    dummy.update_body();
    print_stomach_contents( dummy, print_tests );
    hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
        print_stomach_contents( dummy, print_tests );
        printf( "eat 2 beansnrice\n" );
    }
    CHECK( hunger_time <= 240 );
    CHECK( hunger_time >= 210 );
    f = item( "beansnrice" );
    dummy.eat( f );
    f = item( "beansnrice" );
    dummy.eat( f );
    dummy.update_body();
    print_stomach_contents( dummy, print_tests );
    hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
    }
    CHECK( hunger_time <= 240 );
    CHECK( hunger_time >= 210 );
    if( print_tests ) {
        print_stomach_contents( dummy, print_tests );
        printf( "eat 16 veggy\n" );
    }
    for( int i = 0; i < 16; i++ ) {
        f = item( "veggy" );
        dummy.eat( f );
    }
    dummy.update_body();
    print_stomach_contents( dummy, print_tests );
    hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
        print_stomach_contents( dummy, print_tests );
    }
    CHECK( hunger_time <= 300 );
    CHECK( hunger_time >= 240 );
    if( print_tests ) {
        printf( "eat 16 veggy with extreme metabolism\n" );
    }
    while( !( dummy.has_trait( trait_id( "HUNGER3" ) ) ) ) {
        dummy.mutate_towards( trait_id( "HUNGER3" ) );
    }
    for( int i = 0; i < 16; i++ ) {
        f = item( "veggy" );
        dummy.eat( f );
    }
    dummy.update_body();
    print_stomach_contents( dummy, print_tests );
    hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
        print_stomach_contents( dummy, print_tests );
    }
    CHECK( hunger_time <= 210 );
    CHECK( hunger_time >= 120 );
}
