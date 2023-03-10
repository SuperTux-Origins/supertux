
import("/levels/preload_worldselect.nut");

fade_time <- 0.35;

// ============================================================================
//   AMBIENT LIGHT
// ============================================================================

if("ambient_r" in state && "ambient_g" in state && "ambient_b" in state){
  worldmap.settings.fade_to_ambient_light(state.ambient_r,
                                          state.ambient_g,
                                          state.ambient_b,
                                          0);
}

// ============================================================================
//   UNDERGROUND
// ============================================================================

if(! ("underground" in state)){
	state.underground <- false;
  print("[DEBUG] Underground state initialized\n");
}

function go_underground(under){
  Underground_cover.fade(under ? 0 : 1, fade_time);
  Land_foreground_1.fade(under ? 0 : 1, fade_time);
  Land_foreground_2.fade(under ? 0 : 1, fade_time);
  Land_foreground_3.fade(under ? 0 : 1, fade_time);
  Land_foreground_4.fade(under ? 0 : 1, fade_time);
  Underground_mask.fade(under ? 1 : 0, fade_time);
  Underground_darkness.fade(under ? 1 : 0, fade_time);
  state.underground <- under;
}

go_underground(state.underground);


// ============================================================================
//   SECRET AREAS
// ============================================================================

if(! ("iv_secret" in state)){
	state.iv_secret <- false;
  print("[DEBUG] 'Icy Valley' secret road initialized\n");
}

function toggle_secret_road(tilemap, enabled){
  tilemap.fade(enabled ? 1 : 0, fade_time);
  tilemap.set_solid(enabled);
}

toggle_secret_road(iv_secret, state.iv_secret);
