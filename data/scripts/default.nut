/**
 * This script gets loaded into the squirrel root vm in supertux. So functions
 * and variables you define here can be used in all threads
 */

//Create Level table
Level <- {
  finish=Level_finish,
  spawn=Level_spawn,
  toggle_pause=Level_toggle_pause,
  edit=Level_edit
};


function end_level()
{
  play_music("music/misc/leveldone.ogg");
  Tux.deactivate();
  wait(6);
  Effect.fade_out(2);
  wait(2);
  Level.finish(true);
}

function exit_worldmap()
{
  wait_for_screenswitch();
  exit_screen();
}

function println(val)
{
  print(val);
  print("\n");
}

if(! ("state" in this))
  state <- {};

