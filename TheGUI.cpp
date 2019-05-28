/*
 * Sample code from android application https://play.google.com/store/apps/details?id=com.anypackagehost.unproject_rubic
*/

#include "TheGUI.h"
#include "GUI/all_controls.h"
#include "GameControl.h"
#include "Records.h"


class MenuButton:public CustomButton
{
public:
    MenuButton(Layout &l):CustomButton(l,FTCenter,FTTop)
    {
        Size sz=getTextSize("M").expandedOn(GUIStyle::current().text_padding);
        sz._w=sz._h=std::max(sz._w,sz._h);
        setFixedSize(sz);
    }
    void onSetupControlStates(WindowRenderer::StateMachine &sm)override
    {
        ASSERT(rect().width()==rect().height());
        float r=rect().width()/2;
        const GLColor norm_color=GUIStyle::current().frame_color;
        int shape_n=0;
        sm.updateHollowCircle(shape_n++,rect().center(),r);
        r-=GUIStyle::current().text_padding;
        float len=r*sqrt(2);
        float offset=(rect().width()-len)/2;
        float h_step=len/5;
        for(int i=0;i<3;i++)
        {
            pt2d bl=rect().corner(Rect::BottomLeft)+vc2d{offset,offset+i*2*h_step};
            pt2d tr{bl.x+len,bl.y+h_step};
            sm.updateRectShape(shape_n++,ShapeType::SolidRect,Rect::from2Edges(bl,tr));
        }
        for(int i=0;i<shape_n;i++)
            sm.setupStates(i,norm_color);
    }
};

class TimerLabel: public DynamicTextWindow
{
public:
    TimerLabel(Layout &l):DynamicTextWindow("00:00",l,FTCenter,FTBottom)
    {
        adjustSizeToValues({"100:00","12:22"});
    }
protected:
    void onTimeUpdate(float delta)override
    {
        if(!GameControl::isInGame())
            setText("Solved");
        else
            setText(secToTimeStr(settings().tmp_timer));
    }
    void draw()const override
    {
        if(!GameControl::isShufflingTheCube())
        {
            if(!GameControl::isInGame()||settings().show_timer)
                DynamicTextWindow::draw();
        }
    }
};


///all GUI controls with handlers are here:
TheGUI::TheGUI():_concurency(ViewsConcurency::None)
{
    HorizontalLayout* persp_setup_overlay=new HorizontalLayout(desktop(),FTStretch,FTStretch);
    VerticalLayout* overlay=new VerticalLayout(desktop(),FTCenter,FTStretch);
    overlay->_draw_border=false;
    overlay->_outer_border=overlay->_inner_border=0;

    MenuButton* menu_btn=new MenuButton(*overlay);
    new TimerLabel(*overlay);

    std::function<void(SubWindow*)> show=[=](SubWindow* w)
    {
        GameControl::gui_is_ready_for_ad=w!=overlay;
        ASSERT(dynamic_cast<Dialog*>(w)==NULL);
        if(dynamic_cast<RecordsTable*>(w))
            GameControl::gui_is_ready_for_ad=false;
        desktop().setWindow(*w);
        if(w==persp_setup_overlay)
            _concurency=ViewsConcurency::ExclusiveMouseHandling;
        else if(w!=overlay)
            blockAllOtherViews();
        else
            unblockAllOtherViews();
    };
    Dialog::setOnStartDialogHandler([](){GameControl::gui_is_ready_for_ad=false;});

    VoidFunc startTheGame=[=]()
    {
        show(overlay);
        GameControl::startTheGame();
    };

    //MENU:
    Menu* main_menu=new Menu(desktop(),FTCenter,FTCenter);
    main_menu->addItem("NEW GAME");
    main_menu->addItem("NEW GAME...");
    main_menu->addItem("OPTIONS");
    Menu::Item* menu_back=main_menu->addItem("BACK");
    //main_menu->addItem("TEST");
    main_menu->addItem("ABOUT");
    main_menu->addItem("QUIT");

    VoidFunc showMainMenu=[show,main_menu,menu_back](){
        show(main_menu);
        menu_back->setEnabled(GameControl::isInGame());
    };

    menu_btn->setClickHandler([=]()
    {   showMainMenu();});

    //New custom game:
    HeaderLayout* new_game_params=new HeaderLayout("Parameters",desktop(),FTCenter,FTCenter);
    newSpinBox(settings().shuffle_steps,"Shuffle turns:",{1,50},1,*new_game_params);
    newSpinBox(settings().cube_size,"Cube size:",{2,3},1,*new_game_params);
    HorButtonsGroup* btn_group=new HorButtonsGroup(*new_game_params);
    btn_group->addBtn("Back")->setClickHandler([=](){showMainMenu();});
    btn_group->addBtn("Start")->setClickHandler([=](){startTheGame();});

    //Options:
    HeaderLayout* opts_header=new HeaderLayout("Options",desktop(),FTCenter,FTCenter);
    VerticalLayout* options=new VerticalLayout(*opts_header,FTStretch,FTStretch);
    options->_outer_border=options->_inner_border=0;
    options->_draw_border=false;
    newSpinBox(settings().animation_speed, "Animation speed:",{1.f,20.f},1.f,*options)->setValueFormatCorrection(
            [](const Str &value){ return value.substr(0,value.size()-3);});
    new BoolEnumControl(settings().show_shuffle_animation,"Suffle animation:",{"OFF","ON"},*options);
    new BoolEnumControl(settings().show_timer,"Timer:",{"HIDDEN","VISIBLE"},*options);
    new BoolEnumControl(settings().use_flat_view,"View mode:  ",{"PERSPECTIVE","ISOMERTY(2D)"},*options);

    //Perspective settings
    HeaderLayout* persp_settings=new HeaderLayout("Perspective options",*persp_setup_overlay,FTCenter,FTCenter);
    persp_settings->_draw_bg_frame=true;
    newSpinBox(settings().perspective_distortion,"Perspective distortion:",{0.2f,1.4f},0.05f,*persp_settings,FTStretch,FTTop);
    newSpinBox(settings().main_cube_alfa, "Right cube opacity:",{0,1},0.05f,*persp_settings,FTStretch,FTTop);
    newSpinBox(settings().secondary_cube_alfa, "Left cube opacity:",{0,1},0.05f,*persp_settings,FTStretch,FTTop);
    (new Button("Accept",*persp_settings,FTStretch,FTTop))->setClickHandler([=]()
    {GameControl::rubic_mode=RubicViewMode::Normal; show(opts_header);});

    //Options -continue:
    (new Button("Perspective>>",*options,FTStretch,FTTop))->setClickHandler([=]()
    {GameControl::rubic_mode=RubicViewMode::SetupPerspective; show(persp_setup_overlay);});
    //+new DualEnumBox("Cube place:",{"LEFT","RIGHT"},*options);
    //+show clock
    HorButtonsGroup* opts_btn_group=new HorButtonsGroup(*options);
    opts_btn_group->addBtn("Accept")->setClickHandler([=](){showMainMenu();});

    //Menu handlers:
    main_menu->setItemHandler([=](ItemId item)
    {
        if(item=="OPTIONS")
            show(opts_header);
        else if(item=="BACK")
            show(overlay);
        else if(item=="NEW GAME")
            startTheGame();
        else if(item=="NEW GAME...")
            show(new_game_params);
        else if(item=="QUIT")
            GameControl::quit();
        else if(item=="ABOUT")
            Dialog::execute("Credits(3rd party)","This software uses part of the\n"
                                                 "Freetype library(www.freetype.org),\n"
                                                 "under the terms of the FT License."
                                                 ,{"OK"});
        else if(item=="TEST")
            Dialog::execute("!",DebugVars::getVarsString(),{"OK"});
        else
            UNEXPECTED;
    });

    //After win:
    RecordsTable* rec_tab=new RecordsTable(desktop(),FTCenter,FTCenter);
    rec_tab->_close_btn->setClickHandler([=](){show(overlay);});
    GameControl::setVictoryHandler([=]()
    {
        blockAllOtherViews();
        info<<"you've lost"<<secToTimeStr(settings().tmp_timer);
        Dialog::execute(" *Congratulations* ","You win!",{"OK","RECORDS"},[=](ButtonId btn)
        {
            if(btn=="RECORDS")
            {
                rec_tab->reArrange();
                show(rec_tab);
            }
            else
                unblockAllOtherViews();
        });
    });
    if(settings().need_update_notification)//update 2.0
    {
        blockAllOtherViews();
        Dialog::execute("*A new view is available*","Views can be switched in \n Options -> View mode. Switch \nto the new view now?",
                {"Switch","Don't switch"},[=](ButtonId btn)
        {
            settings().use_flat_view=btn!="Switch";
            showMainMenu();
        });
    }
    else
        showMainMenu();
}
//todo: clock, bgcolor, brightness, motion back while in motion

ViewsConcurency TheGUI::viewConcurency() const
{
    return _concurency;
}

void TheGUI::blockAllOtherViews()
{
    _concurency=ViewsConcurency::ExclusiveDrawing|ViewsConcurency::ExclusiveMouseHandling;
}

void TheGUI::unblockAllOtherViews()
{
    _concurency=ViewsConcurency::None;
}
