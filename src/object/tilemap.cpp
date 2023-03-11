//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "object/tilemap.hpp"

#include <tuple>

#include "collision/collision_movement_manager.hpp"
#include "collision/collision_object.hpp"
#include "supertux/autotile.hpp"
#include "supertux/debug.hpp"
#include "supertux/globals.hpp"
#include "supertux/sector.hpp"
#include "supertux/tile.hpp"
#include "supertux/tile_set.hpp"
#include "util/reader.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"
#include "video/drawing_context.hpp"
#include "video/layer.hpp"
#include "video/surface.hpp"
#include "worldmap/worldmap.hpp"

TileMap::TileMap(TileSet const*new_tileset) :
  ExposedObject<TileMap, scripting::TileMap>(this),
  PathObject(),
  m_tileset(new_tileset),
  m_tiles(),
  m_real_solid(false),
  m_effective_solid(false),
  m_speed_x(1),
  m_speed_y(1),
  m_width(0),
  m_height(0),
  m_z_pos(0),
  m_offset(Vector(0,0)),
  m_movement(0,0),
  m_objects_hit_bottom(),
  m_ground_movement_manager(nullptr),
  m_flip(NO_FLIP),
  m_alpha(1.0),
  m_current_alpha(1.0),
  m_remaining_fade_time(0),
  m_tint(1, 1, 1),
  m_current_tint(1, 1, 1),
  m_remaining_tint_fade_time(0),
  m_draw_target(DrawingTarget::COLORMAP),
  m_new_size_x(0),
  m_new_size_y(0),
  m_new_offset_x(0),
  m_new_offset_y(0),
  m_add_path(false),
  m_starting_node(0)
{
}

TileMap::TileMap(TileSet const*tileset_, ReaderMapping const& reader) :
  GameObject(reader),
  ExposedObject<TileMap, scripting::TileMap>(this),
  PathObject(),
  m_tileset(tileset_),
  m_tiles(),
  m_real_solid(false),
  m_effective_solid(false),
  m_speed_x(1),
  m_speed_y(1),
  m_width(-1),
  m_height(-1),
  m_z_pos(0),
  m_offset(Vector(0,0)),
  m_movement(Vector(0,0)),
  m_objects_hit_bottom(),
  m_ground_movement_manager(nullptr),
  m_flip(NO_FLIP),
  m_alpha(1.0),
  m_current_alpha(1.0),
  m_remaining_fade_time(0),
  m_tint(1, 1, 1),
  m_current_tint(1, 1, 1),
  m_remaining_tint_fade_time(0),
  m_draw_target(DrawingTarget::COLORMAP),
  m_new_size_x(0),
  m_new_size_y(0),
  m_new_offset_x(0),
  m_new_offset_y(0),
  m_add_path(false),
  m_starting_node(0)
{
  assert(m_tileset);

  reader.read("solid",  m_real_solid);

  bool backward_compatibility_fudge = false;

  if (!reader.read("speed-x", m_speed_x)) {
    if (reader.read("speed",  m_speed_x)) {
      backward_compatibility_fudge = true;
    }
  }

  if (!reader.read("speed-y", m_speed_y)) {
    if (backward_compatibility_fudge) {
      m_speed_y = m_speed_x;
    }
  }

  m_z_pos = reader_get_layer(reader, 0);

  if (m_real_solid && ((m_speed_x != 1) || (m_speed_y != 1))) {
    log_warning("Speed of solid tilemap is not 1. fixing");
    m_speed_x = 1;
    m_speed_y = 1;
  }

  m_starting_node = reader.get("starting-node", 0);

  init_path(reader, false);

  std::string draw_target_s = reader.get("draw-target", std::string("normal"));
  if (draw_target_s == "normal") m_draw_target = DrawingTarget::COLORMAP;
  if (draw_target_s == "lightmap") m_draw_target = DrawingTarget::LIGHTMAP;

  if (reader.read("alpha", m_alpha)) {
    m_current_alpha = m_alpha;
  }

  std::vector<float> vColor;
  if (reader.read("tint", vColor)) {
    m_current_tint = Color(vColor);
    m_tint = m_current_tint;
  }

  /* Initialize effective_solid based on real_solid and current_alpha. */
  m_effective_solid = m_real_solid;
  update_effective_solid ();

  reader.read("width", m_width);
  reader.read("height", m_height);
  if (m_width < 0 || m_height < 0) {
    //throw std::runtime_error("Invalid/No width/height specified in tilemap.");
    m_width = 0;
    m_height = 0;
    m_tiles.clear();
    resize(static_cast<int>(Sector::get().get_width() / 32.0f),
           static_cast<int>(Sector::get().get_height() / 32.0f));
  } else {
    std::vector<int> tmp_tiles;
    if (!reader.read("tiles", tmp_tiles))
      throw std::runtime_error("No tiles in tilemap.");
    m_tiles.reserve(tmp_tiles.size());
    std::transform(tmp_tiles.begin(), tmp_tiles.end(),
                   std::back_inserter(m_tiles),
                   [](int x) { return static_cast<uint32_t>(x); });

    if (int(m_tiles.size()) != m_width * m_height) {
      throw std::runtime_error("wrong number of tiles in tilemap.");
    }
  }

  bool empty = true;

  // make sure all tiles used on the tilemap are loaded and tilemap isn't empty
  for (auto const& tile : m_tiles) {
    if (tile != 0) {
      empty = false;
    }

    m_tileset->get(tile);
  }

  if (empty)
  {
    log_info("Tilemap '{}', z-pos '{}' is empty.", get_name(), m_z_pos);
  }
}

void
TileMap::finish_construction()
{
  if (get_path() && get_path()->get_nodes().size() > 0) {
    if (m_starting_node >= static_cast<int>(get_path()->get_nodes().size()))
      m_starting_node = static_cast<int>(get_path()->get_nodes().size()) - 1;

    set_offset(m_path_handle.get_pos(get_size() * 32, get_path()->get_nodes()[m_starting_node].position));
    get_walker()->jump_to_node(m_starting_node);
  }

  m_add_path = get_walker() && get_path() && get_path()->is_valid();
}

TileMap::~TileMap()
{
}

void
TileMap::float_channel(float target, float &current, float remaining_time, float dt_sec)
{
  float amt = (target - current) / (remaining_time / dt_sec);
  if (amt > 0) current = std::min(current + amt, target);
  if (amt < 0) current = std::max(current + amt, target);
}

void
TileMap::apply_offset_x(int fill_id, int xoffset)
{
  if (!xoffset)
    return;
  for (int y = 0; y < m_height; y++) {
    for (int x = 0; x < m_width; x++) {
      int X = (xoffset < 0) ? x : (m_width - x - 1);
      if (X - xoffset < 0 || X - xoffset >= m_width) {
        m_tiles[y * m_width + X] = fill_id;
      } else {
        m_tiles[y * m_width + X] = m_tiles[y * m_width + X - xoffset];
      }
    }
  }
}

void
TileMap::apply_offset_y(int fill_id, int yoffset)
{
  if (!yoffset)
    return;
  for (int y = 0; y < m_height; y++) {
    int Y = (yoffset < 0) ? y : (m_height - y - 1);
    for (int x = 0; x < m_width; x++) {
      if (Y - yoffset < 0 || Y - yoffset >= m_height) {
        m_tiles[Y * m_width + x] = fill_id;
      } else {
        m_tiles[Y * m_width + x] = m_tiles[(Y - yoffset) * m_width + x];
      }
    }
  }
}

void
TileMap::update(float dt_sec)
{
  // handle tilemap fading
  if (m_current_alpha != m_alpha) {
    m_remaining_fade_time = std::max(0.0f, m_remaining_fade_time - dt_sec);
    if (m_remaining_fade_time == 0.0f) {
      m_current_alpha = m_alpha;
    } else {
      float_channel(m_alpha, m_current_alpha, m_remaining_fade_time, dt_sec);
    }
    update_effective_solid ();
  }

  // handle tint fading
  if (m_current_tint.red != m_tint.red || m_current_tint.green != m_tint.green ||
      m_current_tint.blue != m_tint.blue || m_current_tint.alpha != m_tint.alpha) {

    m_remaining_tint_fade_time = std::max(0.0f, m_remaining_tint_fade_time - dt_sec);
    if (m_remaining_tint_fade_time == 0.0f) {
      m_current_tint = m_tint;
    } else {
      float_channel(m_tint.red  , m_current_tint.red  , m_remaining_tint_fade_time, dt_sec);
      float_channel(m_tint.green, m_current_tint.green, m_remaining_tint_fade_time, dt_sec);
      float_channel(m_tint.blue , m_current_tint.blue , m_remaining_tint_fade_time, dt_sec);
      float_channel(m_tint.alpha, m_current_tint.alpha, m_remaining_tint_fade_time, dt_sec);
    }
  }

  m_movement = Vector(0,0);
  // if we have a path to follow, follow it
  if (get_walker()) {
    get_walker()->update(dt_sec);
    Vector v = get_walker()->get_pos(get_size() * 32, m_path_handle);
    if (get_path() && get_path()->is_valid()) {
      m_movement = v - get_offset();
      set_offset(v);
      if (m_ground_movement_manager != nullptr) {
        for (CollisionObject* other_object : m_objects_hit_bottom) {
          m_ground_movement_manager->register_movement(*this, *other_object, m_movement);
          other_object->propagate_movement(m_movement);
        }
      }
    } else {
      set_offset(m_path_handle.get_pos(get_size() * 32, Vector(0, 0)));
    }
  }

  m_objects_hit_bottom.clear();
}

void
TileMap::draw(DrawingContext& context)
{
  // skip draw if current opacity is 0.0
  if (m_current_alpha == 0.0f) return;

  context.push_transform();

  if (m_flip != NO_FLIP) context.set_flip(m_flip);

  if (m_current_alpha != 1.0f) {
    context.set_alpha(m_current_alpha);
  }

  const float trans_x = context.get_translation().x;
  const float trans_y = context.get_translation().y;

  context.set_translation(Vector(trans_x * m_speed_x,
                                 trans_y * m_speed_y));

  Rectf draw_rect = context.get_cliprect();
  Rect t_draw_rect = get_tiles_overlapping(draw_rect);
  Vector start = get_tile_position(t_draw_rect.left, t_draw_rect.top);

  Vector pos(0.0f, 0.0f);
  int tx, ty;

  std::unordered_map<SurfacePtr,
                     std::tuple<std::vector<Rectf>,
                                std::vector<Rectf>>> batches;

  for (pos.x = start.x, tx = t_draw_rect.left; tx < t_draw_rect.right; pos.x += 32, ++tx) {
    for (pos.y = start.y, ty = t_draw_rect.top; ty < t_draw_rect.bottom; pos.y += 32, ++ty) {
      int index = ty*m_width + tx;
      assert (index >= 0);
      assert (index < (m_width * m_height));

      if (m_tiles[index] == 0) continue;
      Tile const& tile = m_tileset->get(m_tiles[index]);

      if (g_debug.show_collision_rects) {
        tile.draw_debug(context.color(), pos, LAYER_FOREGROUND1);
      }

      SurfacePtr const& surface = tile.get_current_surface();
      if (surface) {
        std::get<0>(batches[surface]).emplace_back(surface->get_region());
        std::get<1>(batches[surface]).emplace_back(pos,
                                                   Sizef(static_cast<float>(surface->get_width()),
                                                         static_cast<float>(surface->get_height())));
      }
    }
  }

  Canvas& canvas = context.get_canvas(m_draw_target);

  for (auto& it : batches)
  {
    SurfacePtr const& surface = it.first;
    if (surface) {
      canvas.draw_surface_batch(surface,
                                std::move(std::get<0>(it.second)),
                                std::move(std::get<1>(it.second)),
                                m_current_tint, m_z_pos);
    }
  }

  context.pop_transform();
}

void
TileMap::goto_node(int node_no)
{
  if (!get_walker()) return;
  get_walker()->goto_node(node_no);
}

void
TileMap::start_moving()
{
  if (!get_walker()) return;
  get_walker()->start_moving();
}

void
TileMap::stop_moving()
{
  if (!get_walker()) return;
  get_walker()->stop_moving();
}

void
TileMap::set(int newwidth, int newheight, const std::vector<unsigned int>&newt,
             int new_z_pos, bool newsolid)
{
  if (int(newt.size()) != newwidth * newheight)
    throw std::runtime_error("Wrong tilecount count.");

  m_width  = newwidth;
  m_height = newheight;

  m_tiles.resize(newt.size());
  m_tiles = newt;

  if (new_z_pos > (LAYER_GUI - 100))
    m_z_pos = LAYER_GUI - 100;
  else
    m_z_pos  = new_z_pos;
  m_real_solid  = newsolid;
  update_effective_solid ();

  // make sure all tiles are loaded
  for (auto const& tile : m_tiles)
    m_tileset->get(tile);
}

void
TileMap::resize(int new_width, int new_height, int fill_id,
                int xoffset, int yoffset)
{
  bool offset_finished_x = false;
  bool offset_finished_y = false;
  if (xoffset < 0 && new_width - m_width < 0)
  {
    apply_offset_x(fill_id, xoffset);
    offset_finished_x = true;
  }
  if (yoffset < 0 && new_height - m_height < 0)
  {
    apply_offset_y(fill_id, yoffset);
    offset_finished_y = true;
  }
  if (new_width < m_width) {
    // remap tiles for new width
    for (int y = 0; y < m_height && y < new_height; ++y) {
      for (int x = 0; x < new_width; ++x) {
        m_tiles[y * new_width + x] = m_tiles[y * m_width + x];
      }
    }
  }

  m_tiles.resize(new_width * new_height, fill_id);

  if (new_width > m_width) {
    // remap tiles
    for (int y = std::min(m_height, new_height)-1; y >= 0; --y) {
      for (int x = new_width-1; x >= 0; --x) {
        if (x >= m_width) {
          m_tiles[y * new_width + x] = fill_id;
          continue;
        }

        m_tiles[y * new_width + x] = m_tiles[y * m_width + x];
      }
    }
  }
  m_height = new_height;
  m_width = new_width;
  if (!offset_finished_x)
    apply_offset_x(fill_id, xoffset);
  if (!offset_finished_y)
    apply_offset_y(fill_id, yoffset);
}

void TileMap::resize(Size const& newsize, Size const& resize_offset) {
  resize(newsize.width, newsize.height, 0, resize_offset.width, resize_offset.height);
}

Rect
TileMap::get_tiles_overlapping(Rectf const&rect) const
{
  Rectf rect2 = rect;
  rect2.move(-m_offset);

  int t_left   = std::max(0     , int(floorf(rect2.get_left  () / 32)));
  int t_right  = std::min(m_width , int(ceilf (rect2.get_right () / 32)));
  int t_top    = std::max(0     , int(floorf(rect2.get_top   () / 32)));
  int t_bottom = std::min(m_height, int(ceilf (rect2.get_bottom() / 32)));
  return Rect(t_left, t_top, t_right, t_bottom);
}

void
TileMap::hits_object_bottom(CollisionObject& object)
{
  m_objects_hit_bottom.insert(&object);
}

void
TileMap::notify_object_removal(CollisionObject* other)
{
  m_objects_hit_bottom.erase(other);
}

void
TileMap::set_solid(bool solid)
{
  m_real_solid = solid;
  update_effective_solid ();
}

uint32_t
TileMap::get_tile_id(int x, int y) const
{
  if (x < 0) x = 0;
  if (x >= m_width) x = m_width - 1;
  if (y < 0) y = 0;
  if (y >= m_height) y = m_height - 1;

  if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
    //log_warning("tile outside tilemap requested");
    return 0;
  }

  return m_tiles[y*m_width + x];
}

bool
TileMap::is_outside_bounds(Vector const& pos) const
{
  auto pos_ = (pos - m_offset) / 32.0f;
  float width = static_cast<float>(m_width);
  float height = static_cast<float>(m_height);
  return pos_.x < 0 || pos_.x >= width || pos_.y < 0 || pos_.y >= height;
}

Tile const&
TileMap::get_tile(int x, int y) const
{
  uint32_t id = get_tile_id(x, y);
  return m_tileset->get(id);
}

uint32_t
TileMap::get_tile_id_at(Vector const& pos) const
{
  Vector xy = (pos - m_offset) / 32.0f;
  return get_tile_id(int(xy.x), int(xy.y));
}

Tile const&
TileMap::get_tile_at(Vector const& pos) const
{
  uint32_t id = get_tile_id_at(pos);
  return m_tileset->get(id);
}

void
TileMap::change(int x, int y, uint32_t newtile)
{
  assert(x >= 0 && x < m_width && y >= 0 && y < m_height);
  m_tiles[y*m_width + x] = newtile;
}

void
TileMap::change_at(Vector const& pos, uint32_t newtile)
{
  Vector xy = (pos - m_offset) / 32.0f;
  change(int(xy.x), int(xy.y), newtile);
}

void
TileMap::change_all(uint32_t oldtile, uint32_t newtile)
{
  for (int x = 0; x < get_width(); x++) {
    for (int y = 0; y < get_height(); y++) {
      if (get_tile_id(x,y) != oldtile)
        continue;

      change(x,y,newtile);
    }
  }
}

void
TileMap::autotile(int x, int y, uint32_t tile)
{
  if (x < 0 || x >= m_width || y < 0 || y >= m_height)
    return;

  uint32_t current_tile = m_tiles[y*m_width + x];
  AutotileSet* curr_set;
  if (current_tile == 0)
  {
    // Special case 1 : If the tile is empty, check if we can use a non-solid
    // tile from the currently selected tile's autotile set (if any).
    curr_set = m_tileset->get_autotileset_from_tile(tile);
  }
  else if (m_tileset->get_autotileset_from_tile(tile) != nullptr &&
      m_tileset->get_autotileset_from_tile(tile)->is_member(current_tile))
  {
    // Special case 2 : If the tile is in multiple autotilesets, check if it
    // is in the same tileset as the selected tile. (Example : tile 47)
    curr_set = m_tileset->get_autotileset_from_tile(tile);
  }
  else
  {
    curr_set = m_tileset->get_autotileset_from_tile(current_tile);
  }

  // If tile is not autotileable, abort
  // If tile is from a corner autotileset, abort as well
  if (curr_set == nullptr)
  {
    return;
  }

  uint32_t realtile = curr_set->get_autotile(current_tile,
    curr_set->is_solid(get_tile_id(x-1, y-1)),
    curr_set->is_solid(get_tile_id(x  , y-1)),
    curr_set->is_solid(get_tile_id(x+1, y-1)),
    curr_set->is_solid(get_tile_id(x-1, y  )),
    curr_set->is_solid(get_tile_id(x  , y  )),
    curr_set->is_solid(get_tile_id(x+1, y  )),
    curr_set->is_solid(get_tile_id(x-1, y+1)),
    curr_set->is_solid(get_tile_id(x  , y+1)),
    curr_set->is_solid(get_tile_id(x+1, y+1)),
    x, y);

  m_tiles[y*m_width + x] = realtile;
}

void
TileMap::autotile_corner(int x, int y, uint32_t tile, AutotileCornerOperation op)
{
  if (x < 0 || x >= m_width || y < 0 || y >= m_height)
    return;

  if (!m_tileset->get_autotileset_from_tile(tile)->is_corner())
    return;

  AutotileSet* curr_set = m_tileset->get_autotileset_from_tile(tile);

  // If tile is not autotileable, abort
  if (curr_set == nullptr)
  {
    return;
  }

  // If tile is not empty or already of the appropriate tileset, abort
  uint32_t current_tile = m_tiles[y*m_width + x];
  if (current_tile != 0 && (m_tileset->get_autotileset_from_tile(tile) != nullptr
      && !m_tileset->get_autotileset_from_tile(tile)->is_member(current_tile)))
  {
    return;
  }

  // If the current tile is 0, it will automatically return 0
  uint8_t mask = curr_set->get_mask_from_tile(current_tile);
  if (op == AutotileCornerOperation::REMOVE_TOP_LEFT) mask = static_cast<uint8_t>(mask & 0x07);
  if (op == AutotileCornerOperation::REMOVE_TOP_RIGHT) mask = static_cast<uint8_t>(mask & 0x0B);
  if (op == AutotileCornerOperation::REMOVE_BOTTOM_LEFT) mask = static_cast<uint8_t>(mask & 0x0D);
  if (op == AutotileCornerOperation::REMOVE_BOTTOM_RIGHT) mask = static_cast<uint8_t>(mask & 0x0E);
  if (op == AutotileCornerOperation::ADD_TOP_LEFT) mask = static_cast<uint8_t>(mask | 0x08);
  if (op == AutotileCornerOperation::ADD_TOP_RIGHT) mask = static_cast<uint8_t>(mask | 0x04);
  if (op == AutotileCornerOperation::ADD_BOTTOM_LEFT) mask = static_cast<uint8_t>(mask | 0x02);
  if (op == AutotileCornerOperation::ADD_BOTTOM_RIGHT) mask = static_cast<uint8_t>(mask | 0x01);

  uint32_t realtile = (!mask) ? 0 : curr_set->get_autotile(current_tile,
    (mask & 0x08) != 0,
    false,
    (mask & 0x04) != 0,
    false,
    false,
    false,
    (mask & 0x02) != 0,
    false,
    (mask & 0x01) != 0,
    x, y);

  m_tiles[y*m_width + x] = realtile;
}

bool
TileMap::is_corner(uint32_t tile)
{
  auto* ats = m_tileset->get_autotileset_from_tile(tile);

  return ats && ats->is_corner();
}

void
TileMap::autotile_erase(Vector const& pos, Vector const& corner_pos)
{
  if (pos.x < 0.f || pos.x >= static_cast<float>(m_width) ||
      pos.y < 0.f || pos.y >= static_cast<float>(m_height))
    return;

  if (corner_pos.x < 0.f || corner_pos.x >= static_cast<float>(m_width) ||
      corner_pos.y < 0.f || corner_pos.y >= static_cast<float>(m_height))
    return;

  uint32_t current_tile = m_tiles[static_cast<int>(pos.y)*m_width
                                  + static_cast<int>(pos.x)];

  AutotileSet* curr_set = m_tileset->get_autotileset_from_tile(current_tile);

  if (curr_set && curr_set->is_corner()) {
    int x = static_cast<int>(corner_pos.x), y = static_cast<int>(corner_pos.y);
    autotile_corner(x, y, current_tile, AutotileCornerOperation::REMOVE_TOP_LEFT);
    autotile_corner(x-1, y, current_tile, AutotileCornerOperation::REMOVE_TOP_RIGHT);
    autotile_corner(x, y-1, current_tile, AutotileCornerOperation::REMOVE_BOTTOM_LEFT);
    autotile_corner(x-1, y-1, current_tile, AutotileCornerOperation::REMOVE_BOTTOM_RIGHT);
  }
  else
  {
    int x = static_cast<int>(pos.x), y = static_cast<int>(pos.y);
    m_tiles[y*m_width + x] = 0;

    if (x - 1 >= 0 && y - 1 >= 0 && !is_corner(m_tiles[(y-1)*m_width + x-1])) {
      if (m_tiles[y*m_width + x] == 0)
        autotile(x, y, m_tiles[(y-1)*m_width + x-1]);
      autotile(x-1, y-1, m_tiles[(y-1)*m_width + x-1]);
    }

    if (y - 1 >= 0 && !is_corner(m_tiles[(y-1)*m_width + x])) {
      if (m_tiles[y*m_width + x] == 0)
        autotile(x, y, m_tiles[(y-1)*m_width + x]);
      autotile(x, y-1, m_tiles[(y-1)*m_width + x]);
    }

    if (y - 1 >= 0 && x + 1 < m_width && !is_corner(m_tiles[(y-1)*m_width + x+1])) {
      if (m_tiles[y*m_width + x] == 0)
        autotile(x, y, m_tiles[(y-1)*m_width + x+1]);
      autotile(x+1, y-1, m_tiles[(y-1)*m_width + x+1]);
    }

    if (x - 1 >= 0 && !is_corner(m_tiles[y*m_width + x-1])) {
      if (m_tiles[y*m_width + x] == 0)
        autotile(x, y, m_tiles[y*m_width + x-1]);
      autotile(x-1, y, m_tiles[y*m_width + x-1]);
    }

    if (x + 1 < m_width && !is_corner(m_tiles[y*m_width + x+1])) {
      if (m_tiles[y*m_width + x] == 0)
        autotile(x, y, m_tiles[y*m_width + x+1]);
      autotile(x+1, y, m_tiles[y*m_width + x+1]);
    }

    if (x - 1 >= 0 && y + 1 < m_height && !is_corner(m_tiles[(y+1)*m_width + x-1])) {
      if (m_tiles[y*m_width + x] == 0)
        autotile(x, y, m_tiles[(y+1)*m_width + x-1]);
      autotile(x-1, y+1, m_tiles[(y+1)*m_width + x-1]);
    }

    if (y + 1 < m_height && !is_corner(m_tiles[(y+1)*m_width + x])) {
      if (m_tiles[y*m_width + x] == 0)
        autotile(x, y, m_tiles[(y+1)*m_width + x]);
      autotile(x, y+1, m_tiles[(y+1)*m_width + x]);
    }

    if (y + 1 < m_height && x + 1 < m_width && !is_corner(m_tiles[(y+1)*m_width + x+1])) {
      if (m_tiles[y*m_width + x] == 0)
        autotile(x, y, m_tiles[(y+1)*m_width + x+1]);
      autotile(x+1, y+1, m_tiles[(y+1)*m_width + x+1]);
    }
  }
}

AutotileSet*
TileMap::get_autotileset(uint32_t tile) const
{
  return m_tileset->get_autotileset_from_tile(tile);
}

void
TileMap::fade(float alpha_, float seconds)
{
  m_alpha = alpha_;
  m_remaining_fade_time = seconds;
}

void
TileMap::tint_fade(Color const& new_tint, float seconds)
{
  m_tint = new_tint;
  m_remaining_tint_fade_time = seconds;
}

void
TileMap::set_alpha(float alpha_)
{
  m_alpha = alpha_;
  m_current_alpha = m_alpha;
  m_remaining_fade_time = 0;
  update_effective_solid ();
}

float
TileMap::get_alpha() const
{
  return m_current_alpha;
}

void
TileMap::move_by(Vector const& shift)
{
  if (!get_path()) {
    init_path_pos(m_offset);
    m_add_path = true;
  }
  get_path()->move_by(shift);
  m_offset += shift;
}

void
TileMap::update_effective_solid()
{
  bool old = m_effective_solid;
  if (!m_real_solid)
    m_effective_solid = false;
  else if (m_effective_solid && (m_current_alpha < 0.25f))
    m_effective_solid = false;
  else if (!m_effective_solid && (m_current_alpha >= 0.75f))
    m_effective_solid = true;

  if(Sector::current() != nullptr && old != m_effective_solid)
  {
      Sector::get().update_solid(this);
  } else if(worldmap::WorldMap::current() != nullptr && old != m_effective_solid) {
      worldmap::WorldMap::current()->update_solid(this);
  }
}

void
TileMap::set_tileset(TileSet const* new_tileset)
{
  m_tileset = new_tileset;
}

/* EOF */
