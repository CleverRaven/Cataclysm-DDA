local MOD = {}

mods["MonsterSpecialAttackTest"] = MOD

function say_hello(monster)
  game.add_msg(monster:disp_name()..": Hello!")
  return true;
end

game.register_monattack("SAY_HELLO", say_hello)
