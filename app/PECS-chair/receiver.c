#include "receiver.h"

int32_t timediff = 0;

/* function (msgTable, ip, port)
      print("got")
      pt(msgTable)
      local retTable = {}
      local cmd = 0
      if msgTable["heaters"] ~= nil then
         msgTable["backh"] = msgTable["heaters"]
         msgTable["bottomh"] = msgTable["heaters"]
         msgTable["heaters"] = nil
      end
      if msgTable["fans"] ~= nil then
         msgTable["backf"] = msgTable["fans"]
         msgTable["bottomf"] = msgTable["fans"]
         msgTable["fans"] = nil
      end
      for _, control in pairs(controls) do
         if msgTable[control] ~= nil then
            cmd = msgTable[control]
            if cmd >= 0 and cmd <= 100 then
               fnmap[control](instmap[control], cmd)
               retTable[control] = "ok"
            else
               retTable[control] = "fail"
            end
         end
      end
      print("returning")
      pt(retTable)
      return retTable
      end */

int actuation_handler(lua_State* L) {
    int i, value_index, cmd, ret_index, set_index;
    lua_newtable(L); // retTable
    ret_index = lua_gettop(L);

    lua_getglobal(L, "ChairSettings");
    set_index = lua_gettop(L);

    // Unpack meta-setting "heaters"
    lua_pushstring(L, "heaters");
    lua_gettable(L, 1);
    value_index = lua_gettop(L);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, "backh");
        lua_pushvalue(L, value_index);
        lua_settable(L, 1);
        lua_pushstring(L, "bottomh");
        lua_pushvalue(L, value_index);
        lua_settable(L, 1);
        lua_pushstring(L, "heaters");
        lua_pushnil(L);
        lua_settable(L, 1);
    }
    lua_settop(L, value_index - 1);

    // Unpack meta-setting "fans"
    lua_pushstring(L, "fans");
    lua_gettable(L, 1);
    value_index = lua_gettop(L);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, "backf");
        lua_pushvalue(L, value_index);
        lua_settable(L, 1);
        lua_pushstring(L, "bottomf");
        lua_pushvalue(L, value_index);
        lua_settable(L, 1);
        lua_pushstring(L, "fans");
        lua_pushnil(L);
        lua_settable(L, 1);
    }

    // Iterate over controls
    for (i = 0; i < 4; i++) {
        switch (i) {
        case 0:
            lua_pushstring(L, "backh");
            break;
        case 1:
            lua_pushstring(L, "bottomh");
            break;
        case 2:
            lua_pushstring(L, "backf");
            break;
        case 3:
            lua_pushstring(L, "bottomf");
            break;
        default:
            printf("Error 0\n");
            continue;
        }
        value_index = lua_gettop(L);
        lua_pushvalue(L, value_index);
        lua_gettable(L, 1);
        if (lua_isnil(L, -1)) {
            lua_settop(L, set_index);
            continue;
        }
        cmd = lua_tointeger(L, -1);
        lua_pushvalue(L, value_index);
        if (cmd >= 0 && cmd <= 100) {
            switch (i) {
            case 0:
            case 1:
                lua_pushstring(L, "setHeater");
                break;
            case 2:
            case 3:
                lua_pushstring(L, "setFan");
                break;
            }
            lua_gettable(L, set_index);
            switch (i) {
            case 0:
                lua_pushnumber(L, BACK_HEATER);
                break;
            case 1:
                lua_pushnumber(L, BOTTOM_HEATER);
                break;
            case 2:
                lua_pushnumber(L, BACK_FAN);
                break;
            case 3:
                lua_pushnumber(L, BOTTOM_FAN);
                break;
            }
            lua_pushnumber(L, cmd);
            lua_call(L, 2, 0);
            lua_pushstring(L, "ok");
        } else {
            lua_pushstring(L, "fail");
        }
        lua_settable(L, ret_index);
    }

    lua_pushvalue(L, ret_index);
    return 1;
}

// takes five bytes as arguments
int bl_handler(lua_State* L) {
    int number = luaL_checkint(L, 5);
    lua_getglobal(L, "ChairSettings");
    int settings_index = lua_gettop(L);
    if (number == 1) {
        lua_pushstring(L, "setHeater");
        lua_gettable(L, settings_index);
        int heater_index = lua_gettop(L);
        lua_pushstring(L, "setFan");
        lua_gettable(L, settings_index);
        int fan_index = lua_gettop(L);

        lua_pushvalue(L, heater_index);
        lua_pushnumber(L, BACK_HEATER);
        lua_pushvalue(L, 1);
        lua_call(L, 2, 0);

        lua_pushvalue(L, heater_index);
        lua_pushnumber(L, BOTTOM_HEATER);
        lua_pushvalue(L, 2);
        lua_call(L, 2, 0);

        lua_pushvalue(L, fan_index);
        lua_pushnumber(L, BACK_FAN);
        lua_pushvalue(L, 3);
        lua_call(L, 2, 0);

        lua_pushvalue(L, fan_index);
        lua_pushnumber(L, BOTTOM_FAN);
        lua_pushvalue(L, 4);
        lua_call(L, 2, 0);
    } else {
        lua_pushlightfunction(L, set_time_diff);
        lua_pushlightfunction(L, bytes_to_timestamp);
        lua_pushvalue(L, 1);
        lua_pushvalue(L, 2);
        lua_pushvalue(L, 3);
        lua_pushvalue(L, 4);
        lua_call(L, 4, 1);
        lua_pushlightfunction(L, get_kernel_secs);
        lua_call(L, 0, 1);
        int diff = lua_tointeger(L, -2) - lua_tointeger(L, -1);
        lua_pop(L, 2);
        lua_pushnumber(L, diff);
        lua_call(L, 1, 0);
    }
    return 0;
}

int to_hex_str(lua_State* L) {
    uint16_t num = luaL_checkint(L, 1);
    char str[5];
    sprintf(str, "%x", num);
    lua_pushlstring(L, str, 4);
    return 1;
}

int get_time(lua_State* L) {
    if (timediff) {
        lua_pushlightfunction(L, get_time_always);
        lua_call(L, 0, 1);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int get_time_always(lua_State* L) {
    lua_pushlightfunction(L, get_kernel_secs);
    lua_call(L, 0, 1);
    int32_t time = (int32_t) lua_tointeger(L, -1) + timediff;
    lua_pop(L, 1);
    lua_pushnumber(L, time);
    return 1;
}

int get_time_diff(lua_State* L) {
    if (timediff) {
        lua_pushnumber(L, timediff);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int set_time_diff(lua_State* L) {
    int32_t newtimediff = (int32_t) luaL_checkint(L, 1);
    if (timediff) {
        timediff = (int32_t) (timediff + ALPHA * newtimediff);
    } else {
        timediff = newtimediff;
    }
    return 0;
}

int compute_time_diff(lua_State* L) {
    int64_t t0 = (int64_t) luaL_checkint(L, 1);
    int64_t t1 = (int64_t) luaL_checkint(L, 2);
    int64_t t2 = (int64_t) luaL_checkint(L, 3);
    int64_t t3 = (int64_t) luaL_checkint(L, 4);
    int64_t diff = ((t1 - t0) + (t2 - t3)) >> 1;
    lua_pushnumber(L, (int32_t) diff);
    return 1;
}
