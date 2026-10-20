#----------------------
# | pocketstation_en.py
# | Description: English text for the PocketStation entries the US release leaves empty (CARDA, CLOAD, NIKI, ADDHERO and the ranch scene), filled into the Dreamcast build by overlay_text.py.
# |              LINES maps overlay or scene, then text-table index, to our own line.
# |              Entries no code draws (CARDA 47, 64, 68, 70, 72, 82, 84; scene 60, 64) stay empty. Braces are the scene text controls of
# |              tools/scenes/strings.py; overlay lines are plain ASCII.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: LINES
# | Params: N/A
# | Returns: N/A
#----------------------

#----------------------
# | LINES
# | Description: The English lines by group and index, as described above.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LINES = {'carda': {29: 'Ring Ring Land',
           33: 'This is not a VMU.',
           48: 'Ring Ring Land requires',
           49: '6 Blocks to download.',
           58: 'Checking VMU...',
           59: 'No VMU.',
           60: 'VMU access failed.',
           61: 'Please do not insert or remove',
           62: 'VMU',
           63: 'VMU is not inserted properly.',
           65: 'Ring Ring Land was',
           66: 'not found.',
           67: 'Items your pet brought back',
           69: 'Bring your pet back home?',
           71: 'Your VMU already has a pet.',
           73: 'Bringing your pet back',
           74: 'Send Ring Ring Land to VMU.',
           75: 'Is this OK?',
           78: 'Ring Ring Land',
           79: 'Sending to VMU failed.',
           80: 'Reading from VMU failed.',
           81: 'Sending Ring Ring Land...',
           83: 'from the VMU...',
           85: 'Ring Ring Land will be',
           86: 'erased from the VMU.',
           87: 'That pet is already home.'},
 'niki': {29: 'Ring Ring Land', 33: 'This is not a VMU.'},
 'addhero': {29: 'Ring Ring Land', 33: 'This is not a VMU.'},
 'cload': {29: 'Ring Ring Land', 33: 'This is not a VMU.'},
 'scene_mhm_alc2': {33: '{clear}Ring Ring Land{finish}',
                    61: 'Have fun, and come back soon!{end}',
                    62: "{color:1}Eggs can't go to Ring Ring Land.{default_color}{newline}Wait until it hatches.{end}",
                    63: "{color:1}Your party pet can't go there.{default_color}{newline}Leave it in the corral first.{end}"}}
