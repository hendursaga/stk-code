//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009-2013 Marianne Gagnon
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "states_screens/story_mode_lobby.hpp"

#include "audio/sfx_manager.hpp"
#include "challenges/unlock_manager.hpp"
#include "config/player_manager.hpp"
#include "config/user_config.hpp"
#include "guiengine/widgets/check_box_widget.hpp"
#include "guiengine/widgets/dynamic_ribbon_widget.hpp"
#include "guiengine/widgets/label_widget.hpp"
#include "guiengine/widgets/list_widget.hpp"
#include "states_screens/dialogs/enter_player_name_dialog.hpp"
#include "states_screens/main_menu_screen.hpp"
#include "states_screens/state_manager.hpp"


using namespace GUIEngine;

DEFINE_SCREEN_SINGLETON( StoryModeLobbyScreen );

// ----------------------------------------------------------------------------

StoryModeLobbyScreen::StoryModeLobbyScreen() : Screen("story_mode_lobby.stkgui")
{
}   // StoryModeLobbyScreen

// ----------------------------------------------------------------------------

void StoryModeLobbyScreen::loadedFromFile()
{

}   // loadedFromFile

// ----------------------------------------------------------------------------

void StoryModeLobbyScreen::init()
{
    m_online_cb = getWidget<CheckBoxWidget>("online");
    assert(m_online_cb);
    m_username_tb = getWidget<TextBoxWidget >("username");
    assert(m_username_tb);
    m_password_tb = getWidget<TextBoxWidget >("password");
    assert(m_password_tb);
    m_password_tb->setPasswordBox(true, L'*');
    m_players = getWidget<DynamicRibbonWidget>("players");
    assert(m_players);

    Screen::init();
    PlayerProfile *player = PlayerManager::getCurrentPlayer();
    if (player)
    {
        //StateManager::get()->resetAndGoToScreen(MainMenuScreen::getInstance());
        //return;
    }

    m_players->clearItems();
    for (unsigned int n=0; n<PlayerManager::get()->getNumPlayers(); n++)
    {
        const PlayerProfile *player = PlayerManager::get()->getPlayer(n);
        if (player->isGuestAccount()) continue;
        std::string s = StringUtils::toString(n);
        m_players->addItem(player->getName(), s, "/karts/nolok/nolokicon.png", 0, 
                       IconButtonWidget::ICON_PATH_TYPE_RELATIVE);
    }

    m_players->updateItemDisplay();

    // Select the first user (the list of users is sorted by usage, so it
    // is the most frequently used user).    
    if (PlayerManager::get()->getNumPlayers()>0)
        selectUser(0);

}   // init

// ----------------------------------------------------------------------------

void StoryModeLobbyScreen::tearDown()
{
    Screen::tearDown();
}   // tearDown

// ----------------------------------------------------------------------------
/** Called when a user is selected. It updates the online checkbox and
 *  entrye fields.
 */
void StoryModeLobbyScreen::selectUser(int index)
{
    PlayerProfile *profile = PlayerManager::get()->getPlayer(index);
    assert(profile);
    getWidget<TextBoxWidget >("username")->setText(profile
                                                   ->getLastOnlineName());
    m_players->setSelection(StringUtils::toString(index), 0, /*focusIt*/true);

    // Last game was not online, so make the offline settings the default
    // (i.e. unckeck online checkbox, and make entry fields invisible).
    if (profile->getLastOnlineName() == "")
    {
        m_online_cb->setState(false);
        makeEntryFieldsVisible(false);
        return;
    }

    // Now last use was with online --> Display the saved data
    makeEntryFieldsVisible(true);
    m_username_tb->setText(profile->getLastOnlineName());

    // And make the password invisible if the session is saved (i.e
    // the user does not need to enter a password).
    if (profile->hasSavedSession())
    {
        m_password_tb->setVisible(false);
        getWidget<LabelWidget>("label_password")->setVisible(false);
    }

}   // selectUser

// ----------------------------------------------------------------------------
/** Make the entry fields either visible or invisible.
 *  \param online Online state, which dicates if the entry fields are
 *         visible (true) or not.
 */
void StoryModeLobbyScreen::makeEntryFieldsVisible(bool online)
{
    getWidget<LabelWidget>("label_username")->setVisible(online);
    m_username_tb->setVisible(online);
    getWidget<LabelWidget>("label_password")->setVisible(online);
    m_password_tb->setVisible(online);
}   // update

// ----------------------------------------------------------------------------
/** Called when the user selects anything on the screen.
 */
void StoryModeLobbyScreen::eventCallback(Widget* widget,
                                         const std::string& name,
                                         const int player_id)
{
    LabelWidget *info = getWidget<LabelWidget>("message");
    info->setText("", true);
    info->setErrorColor();

    if (name == "players")
    {
        // Clicked on a name --> Find the corresponding online data
        // and display them
        const std::string &s_index = getWidget<DynamicRibbonWidget>("players")
                                   ->getSelectionIDString(player_id);
        if (s_index == "") return;  // can happen if the list is empty

        unsigned int id;
        if (StringUtils::fromString(s_index, id))
            selectUser(id);
    }
    else if (name == "online")
    {
        // If online access is not allowed, do not accept an online account
        // but advice the user where to enable this option.
        if (m_online_cb->getState() && UserConfigParams::m_internet_status ==
                                       Online::RequestManager::IPERM_NOT_ALLOWED)
        {
            info->setText(
                "Internet access is disabled, please enable it in the options",
                true);
            sfx_manager->quickSound( "anvil" );
            m_online_cb->setState(false);
        }
        makeEntryFieldsVisible(m_online_cb->getState());
    }
    else if (name == "options")
    {
        const std::string &button =
            getWidget<RibbonWidget>("options")->getSelectionIDString(player_id);
        if (button == "ok" || button == "ok_and_save")
        {
            if (m_online_cb->getState() && m_password_tb->getText() == "")
            {
                info->setText(_("You need to enter a password."),true);
                sfx_manager->quickSound( "anvil" );
                return;
            }

            const std::string &name = m_players->getSelectionIDString(player_id);

            unsigned int id;
            StringUtils::fromString(name, id);
            PlayerProfile *profile = PlayerManager::get()->getPlayer(id);
            PlayerManager::get()->setCurrentPlayer(profile, button=="ok_and_save");
            StateManager::get()->pushScreen(MainMenuScreen::getInstance());
            return;
        }   // button==ok || ok_and_save
        else if (button == "new_user")
        {
            new EnterPlayerNameDialog(this, 0.5f, 0.4f);
        }
    }   // options

    return;


    if (name == "back")
    {
        StateManager::get()->escapePressed();
    }
    else if (name == "gameslots")
    {
        ListWidget* list = getWidget<ListWidget>("gameslots");

        bool slot_found = false;

        for (unsigned int n=0; n<PlayerManager::get()->getNumPlayers(); n++)
        {
            PlayerProfile *player = PlayerManager::get()->getPlayer(n);
            if (list->getSelectionLabel() == player->getName())
            {
                PlayerManager::get()->setCurrentPlayer(player, false);
                slot_found = true;
                break;
            }
        }

        if (!slot_found)
        {
            Log::error("StoryModeLobby",
                       "Cannot find player corresponding to slot '%s'.",
                     core::stringc(list->getSelectionLabel().c_str()).c_str());
        }
        else
        {
//            CheckBoxWidget* cb = getWidget<CheckBoxWidget>("rememberme");
//            if (cb->getState())
            {
//                UserConfigParams::m_default_player = list->getSelectionLabel();
            }
        }

        StateManager::get()->resetAndGoToScreen(MainMenuScreen::getInstance());
    }
}   // eventCallback

// ----------------------------------------------------------------------------

void StoryModeLobbyScreen::unloaded()
{
}   // unloaded

// ----------------------------------------------------------------------------
/** Gets called when a dialog closes. At a first time start of STK the
 *  internet dialog is shown first. Only when this dialog closes is it possible
 *  to open the next dialog, which is the one to create a new player (which
 *  is conventient on a first start).
 */
void StoryModeLobbyScreen::onDialogClose()
{
    // To allow players to exit the game without creating a player, we count
    // how often this function was called. The first time is after the 
    // internet allowed dialog, the 2nd time
    static int number_of_calls = 0;
    number_of_calls++;
    if(PlayerManager::get()->getNumPlayers() == 0)
    {
        // Still 0 players after the enter player dialog was shown
        // --> User wanted to abort, so pop this menu, which will
        // trigger the end of STK.
        if (number_of_calls > 1)
        {
            StateManager::get()->popMenu();
            return;
        }
        new EnterPlayerNameDialog(this, 0.5f, 0.4f);
    }   // getNumPlayers == 0
}   // onDialogClose

// ----------------------------------------------------------------------------
/** This is a callback from the new user dialog.
 */
void StoryModeLobbyScreen::onNewPlayerWithName(const core::stringw& new_name)
{
    init();
    // Select the newly added player
    selectUser(PlayerManager::get()->getNumPlayers() - 1);
        
    return;
}   // onNewPlayerWithName

// -----------------------------------------------------------------------------

