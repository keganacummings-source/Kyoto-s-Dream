#include "PluginEditor.h"
#include "FeatureNames.h"
#include "UpdateService.h"
#include "ModuleRegistry.h"
#include "PresetCodec.h"
#include <algorithm>
#include <cmath>

static int gSelectedCommunityRow=-1;

class KyotoSpxritEditor::FeedModel : public juce::ListBoxModel {
public: explicit FeedModel(KyotoSpxritEditor& e):ed(e){}
    int getNumRows() override { return (int)ed.feed.size(); }
    void paintListBoxItem(int r,juce::Graphics&g,int w,int h,bool sel) override { if(r<0||r>=getNumRows())return; auto&t=ed.feed[(size_t)r]; auto&th=ed.proc.theme(); g.fillAll(sel?th.panel2:th.bg); g.setColour(th.accent); g.setFont(13); g.drawText(t.user+" / "+t.title,10,4,w-20,20,juce::Justification::left); g.setColour(th.textDim); g.setFont(11); g.drawText(t.text.replace("\n", " ").replace("\r", " "),10,25,w-20,h-28,juce::Justification::left,false); if(t.hasAudio)g.drawText("WAV",w-100,5,40,18,juce::Justification::centred); if(t.hasImage)g.drawText("IMG",w-55,5,40,18,juce::Justification::centred); }
private: KyotoSpxritEditor& ed;
};

class KyotoSpxritEditor::ModuleModel : public juce::ListBoxModel {
public: explicit ModuleModel(KyotoSpxritEditor& e):ed(e){}
    int getNumRows() override { return (int)kyoto::ModuleRegistry::builtins().size()+(int)ed.proc.installedModules().size(); }
    void paintListBoxItem(int r,juce::Graphics&g,int w,int h,bool sel) override { auto&th=ed.proc.theme(); g.fillAll(sel?th.panel2:th.bg); auto a=kyoto::ModuleRegistry::builtins(); juce::String name=r<(int)a.size()?a[(size_t)r].name:ed.proc.installedModules()[(size_t)r-a.size()]; g.setColour(th.text); g.drawText(name,10,0,w-20,h,juce::Justification::centredLeft); }
    void selectedRowsChanged(int r) override { auto a=kyoto::ModuleRegistry::builtins(); if(r>=0&&r<(int)a.size()){ ed.proc.addModule(a[(size_t)r].id); ed.syncBuilderFromProcessor(); ed.status.setText("MODULE LOADED: "+a[(size_t)r].name,juce::dontSendNotification); ed.show(KyotoSpxritEditor::Builder); } else if(r>=(int)a.size()){ auto id=ed.proc.installedModules()[(size_t)r-a.size()]; if(ed.proc.loadModule(id)){ ed.syncBuilderFromProcessor(); ed.status.setText("SAVED MODULE LOADED: "+id,juce::dontSendNotification); ed.show(KyotoSpxritEditor::Builder); } } }
private: KyotoSpxritEditor& ed;
};

class KyotoSpxritEditor::CommunityModel : public juce::ListBoxModel {
public: explicit CommunityModel(KyotoSpxritEditor& e):ed(e){}
    int getNumRows() override { return (int)ed.communityItems.size(); }
    void paintListBoxItem(int r,juce::Graphics&g,int w,int h,bool sel) override { if(r<0||r>=getNumRows())return; auto&th=ed.proc.theme(); auto&i=ed.communityItems[(size_t)r]; g.fillAll(sel?th.panel2:th.bg); g.setColour(th.accent); g.setFont(13); g.drawText(i.name,10,3,w-20,20,juce::Justification::left); g.setColour(th.textDim); g.setFont(11); g.drawText("by "+i.author+"  •  "+juce::String(i.downloads)+" uses",10,24,w-20,18,juce::Justification::left); g.setColour(th.text); g.drawText(i.description,10,43,w-20,h-46,juce::Justification::left,false); }
    void selectedRowsChanged(int r) override { gSelectedCommunityRow=r; }
private: KyotoSpxritEditor& ed;
};

class KyotoSpxritEditor::FxRow : public juce::Component {
public:
    FxRow(KyotoSpxritEditor& e,size_t i):ed(e),index(i){
        addAndMakeVisible(effect); addAndMakeVisible(amount); addAndMakeVisible(tone); addAndMakeVisible(motion); addAndMakeVisible(mix); addAndMakeVisible(shape); addAndMakeVisible(remove);
        effect.addItem("Select effect",1); for(int n=0;n<200;n++) effect.addItem(juce::String(n+1)+"  "+juce::String(dm::featureNames[n].data()),n+2);
        for(auto*s:{&amount,&tone,&motion,&mix,&shape}){s->setRange(0,1,0.001);s->setTextBoxStyle(juce::Slider::TextBoxRight,false,48,18);}
        remove.setButtonText("×"); remove.onClick=[this]{auto*editor=&ed; auto i=index; juce::MessageManager::callAsync([editor,i]{editor->proc.removeFxSlot(i); editor->rebuildFxRows();});};
        effect.onChange=[this]{auto c=ed.proc.fxChain();if(index<c.size()){c[index].effect=effect.getSelectedId()-2;ed.proc.setFxChainSlot(index,c[index]);if(!ed.proc.isExpertMode()&&index<KyotoSpxritProcessor::fxSlots)ed.proc.setFxIndex((int)index,c[index].effect);}};
        auto changed=[this](int which,juce::Slider&s){s.onValueChange=[this,which]{auto c=ed.proc.fxChain();if(index>=c.size())return;auto f=c[index];float v=0;switch(which){case 0:v=amount.getValue();break;case 1:v=tone.getValue();break;case 2:v=motion.getValue();break;case 3:v=mix.getValue();break;default:v=shape.getValue();break;}if(which==0)f.amount=v;else if(which==1)f.tone=v;else if(which==2)f.motion=v;else if(which==3)f.mix=v;else f.shape=v;ed.proc.setFxChainSlot(index,f);if(!ed.proc.isExpertMode()&&index<KyotoSpxritProcessor::fxSlots)ed.proc.setFxParam((int)index,which,v);};};
        changed(0,amount);changed(1,tone);changed(2,motion);changed(3,mix);changed(4,shape);
    }
    void sync(){auto c=ed.proc.fxChain();if(index>=c.size())return;auto&f=c[index];effect.setSelectedId(f.effect+2,juce::dontSendNotification);amount.setValue(f.amount,juce::dontSendNotification);tone.setValue(f.tone,juce::dontSendNotification);motion.setValue(f.motion,juce::dontSendNotification);mix.setValue(f.mix,juce::dontSendNotification);shape.setValue(f.shape,juce::dontSendNotification);}
    void resized() override { auto r=getLocalBounds().reduced(2); remove.setBounds(r.removeFromRight(26)); shape.setBounds(r.removeFromRight(82)); mix.setBounds(r.removeFromRight(82)); motion.setBounds(r.removeFromRight(82)); tone.setBounds(r.removeFromRight(82)); amount.setBounds(r.removeFromRight(82)); effect.setBounds(r); }
private:
    KyotoSpxritEditor& ed; size_t index; juce::ComboBox effect; juce::Slider amount,tone,motion,mix,shape; juce::TextButton remove;
};

class KyotoSpxritEditor::BuilderCanvas : public juce::Component {
public:
    explicit BuilderCanvas(KyotoSpxritEditor& e):ed(e){setInterceptsMouseClicks(true,true);}
    void paint(juce::Graphics&g) override {
        auto&th=ed.proc.theme(); g.fillAll(th.panel2); auto r=getLocalBounds().reduced(8);
        if(ed.showGrid.getToggleState()){g.setColour(th.border.withAlpha(0.65f)); for(int x=1;x<10;x++)g.drawVerticalLine(r.getX()+r.getWidth()*x/10.0f,r.getY(),r.getBottom()); for(int y=1;y<10;y++)g.drawHorizontalLine(r.getY()+r.getHeight()*y/10.0f,r.getX(),r.getRight());}
        const auto& l=ed.proc.uiLayout(); for(size_t i=0;i<l.elements.size();++i){auto&e=l.elements[i];auto b=elementBounds(e,r);bool sel=(int)i==selected;g.setColour(i==selected?th.accent:th.border);g.drawRoundedRectangle(b.toFloat(),5.0f,2.0f);g.setColour(th.text);g.setFont(12); if(e.type==kyoto::BuilderElementType::WaveScreen){g.setColour(th.accent.withAlpha(.18f));g.fillRoundedRectangle(b.toFloat(),5);g.setColour(th.accent);g.drawText(kyoto::screenName(e.screen),b.reduced(6),juce::Justification::centred);}else if(e.type==kyoto::BuilderElementType::Text)g.drawText(e.text.isEmpty()?e.label:e.text,b.reduced(6),juce::Justification::centred);else g.drawText(e.label,b.reduced(6),juce::Justification::centred);}
    }
    void mouseDown(const juce::MouseEvent& ev) override { selected=hit(ev.position); ed.selectElement(selected); }
    void mouseDrag(const juce::MouseEvent& ev) override { if(selected<0)return; auto l=ed.proc.uiLayout();if(selected>=(int)l.elements.size())return;auto r=getLocalBounds().reduced(8);auto&e=l.elements[(size_t)selected];e.x=juce::jlimit(0.0f,0.92f,(ev.position.x-r.getX())/(float)r.getWidth());e.y=juce::jlimit(0.0f,0.92f,(ev.position.y-r.getY())/(float)r.getHeight());ed.proc.setUiLayout(l);repaint();ed.elementChanged(); }
private:
    KyotoSpxritEditor& ed; int selected=-1;
    juce::Rectangle<int> elementBounds(const kyoto::UiElement&e,juce::Rectangle<int> r)const{int w=110,h=52;if(e.type==kyoto::BuilderElementType::Dial){w=110;h=70;}else if(e.type==kyoto::BuilderElementType::Slider){w=170;h=42;}else if(e.type==kyoto::BuilderElementType::WaveScreen){w=300;h=150;}else if(e.type==kyoto::BuilderElementType::Text){w=220;h=42;}return {r.getX()+(int)(e.x*r.getWidth()),r.getY()+(int)(e.y*r.getHeight()),w,h};}
    int hit(juce::Point<float> p)const{auto r=getLocalBounds().reduced(8);auto&l=ed.proc.uiLayout();for(int i=(int)l.elements.size()-1;i>=0;--i)if(elementBounds(l.elements[(size_t)i],r).toFloat().contains(p))return i;return -1;}
};

bool KyotoSpxritEditor::isFxBuild() const {
#if defined(KYOTO_IS_FX)
    return true;
#else
    return false;
#endif
}

KyotoSpxritEditor::KyotoSpxritEditor(KyotoSpxritProcessor&p):AudioProcessorEditor(p),proc(p){
    setSize(1220,820);setResizable(true,true);
    for(auto*b:{&home,&selector,&builder,&community,&themesPage,&login,&logout,&update,&refresh,&post,&postWav,&postImage,&saveModule,&uploadCommunity,&loadCommunity,&addFx,&addDial,&addSlider,&addScreen,&addText,&deleteElement,&makeTheme}){addAndMakeVisible(*b);style(*b);}
    for(auto*w:{&expert,&showGrid})addAndMakeVisible(*w); for(auto*c:{&themeBox,&variantBox,&gridBox,&screenBox,&elementBox})addAndMakeVisible(*c);
    for(auto*l:{&title,&status,&modeLabel,&lockLabel,&moduleName,&themeHint,&screenHint})addAndMakeVisible(*l);
    for(auto*t:{&user,&pass,&postText,&saveName,&themeName,&themeAccent,&elementText}){addAndMakeVisible(*t);t->setColour(juce::TextEditor::backgroundColourId,proc.theme().panel2);t->setColour(juce::TextEditor::textColourId,proc.theme().text);} 
    addAndMakeVisible(fxViewport);

    auto configureSlider = [](juce::Slider& slider, double minimum, double maximum, double step)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 18);
        slider.setRange(minimum, maximum, step);
    };

    for (auto* slider : { &macroTone, &macroPunch, &macroSpace, &macroMovement, &macroWidth })
    {
        addAndMakeVisible(*slider);
        configureSlider(*slider, 0.0, 1.0, 0.001);
    }

    configureSlider(oscMix1, 0.0, 1.0, 0.001);
    configureSlider(oscMix2, 0.0, 1.0, 0.001);
    configureSlider(oscMix3, 0.0, 1.0, 0.001);
    configureSlider(detune2, -50.0, 50.0, 0.01);
    configureSlider(detune3, -50.0, 50.0, 0.01);
    configureSlider(cutoff, 0.01, 0.99, 0.001);
    configureSlider(resonance, 0.0, 1.0, 0.001);
    configureSlider(attack, 0.001, 2.0, 0.001);
    configureSlider(decay, 0.001, 2.0, 0.001);
    configureSlider(sustain, 0.0, 1.0, 0.001);
    configureSlider(release, 0.001, 4.0, 0.001);
    configureSlider(noise, 0.0, 1.0, 0.001);
    configureSlider(drive, 0.0, 1.0, 0.001);
    configureSlider(lfoRate, 0.05, 20.0, 0.01);
    configureSlider(lfoDepth, 0.0, 1.0, 0.001);
    configureSlider(octave, -4.0, 4.0, 1.0);

    for (auto* slider : { &oscMix1, &oscMix2, &oscMix3, &detune2, &detune3,
                          &cutoff, &resonance, &attack, &decay, &sustain,
                          &release, &noise, &drive, &lfoRate, &lfoDepth, &octave })
        addAndMakeVisible(*slider);

    auto updateFromControls = [this]
    {
        syncProcessorFromBuilder();
    };

    for (auto* slider : { &macroTone, &macroPunch, &macroSpace, &macroMovement, &macroWidth })
        slider->onValueChange = updateFromControls;

    for (auto* slider : { &oscMix1, &oscMix2, &oscMix3, &detune2, &detune3,
                          &cutoff, &resonance, &attack, &decay, &sustain,
                          &release, &noise, &drive, &lfoRate, &lfoDepth, &octave })
        slider->onValueChange = updateFromControls;

    home.onClick=[this]{show(Home);};selector.onClick=[this]{show(Selector);};builder.onClick=[this]{show(Builder);};community.onClick=[this]{show(Community);};themesPage.onClick=[this]{show(Themes);};login.onClick=[this]{doLogin();};logout.onClick=[this]{proc.logout();show(Home);};refresh.onClick=[this]{refreshFeed();};post.onClick=[this]{postPlain();};postWav.onClick=[this]{postFile(false);};postImage.onClick=[this]{postFile(true);};saveModule.onClick=[this]{saveBuilder();};uploadCommunity.onClick=[this]{uploadCurrent();};loadCommunity.onClick=[this]{loadSelectedCommunity();};update.onClick=[this]{checkUpdate();};expert.onClick=[this]{setExpert(expert.getToggleState());};themeBox.onChange=[this]{applyThemeSelection();};variantBox.onChange=[this]{applyVariant();};makeTheme.onClick=[this]{createTheme();};addFx.onClick=[this]{addFxToChain();};addDial.onClick=[this]{addElement(kyoto::BuilderElementType::Dial);};addSlider.onClick=[this]{addElement(kyoto::BuilderElementType::Slider);};addScreen.onClick=[this]{addElement(kyoto::BuilderElementType::WaveScreen);};addText.onClick=[this]{addElement(kyoto::BuilderElementType::Text);};deleteElement.onClick=[this]{removeElement();};elementBox.onChange=[this]{selectElement(elementBox.getSelectedItemIndex());};elementText.onTextChange=[this]{updateElementText();};gridBox.onChange=[this]{applyGrid();};screenBox.onChange=[this]{applyScreen();};showGrid.onClick=[this]{if(canvas)canvas->repaint();};
    saveModule.setButtonText(isFxBuild()?"SAVE EFFECT":"SAVE INSTRUMENT"); uploadCommunity.setButtonText("UPLOAD COMMUNITY");
    label(title,"KYOTO'S DREAM",28);label(status,"NATIVE VST3 / DREAMSHARE / 200+ EFFECTS",11);label(modeLabel,isFxBuild()?"FX PEDAL / MIXER RACK":"MIDI SYNTH / CHANNEL RACK",12);label(lockLabel,"PLUGIN SELECTOR LOCKED — BUILD OR LOAD A MODULE",15);label(moduleName,isFxBuild()?"EFFECT BUILDER":"INSTRUMENT BUILDER");label(themeHint,"MODULE UI: drag controls around the canvas. Controls have fixed designed sizes.",11);label(screenHint,"WAVE SCREEN TYPE",10);
    user.setTextToShowWhenEmpty("Dream account username",juce::Colours::grey);pass.setTextToShowWhenEmpty("Password",juce::Colours::grey);pass.setPasswordCharacter('*');postText.setMultiLine(true);saveName.setTextToShowWhenEmpty("Instrument / effect name",juce::Colours::grey);themeName.setTextToShowWhenEmpty("Custom theme name",juce::Colours::grey);themeAccent.setTextToShowWhenEmpty("Accent hex, e.g. #c04068",juce::Colours::grey);themeAccent.setText("#c04068",juce::dontSendNotification);elementText.setTextToShowWhenEmpty("Text element content",juce::Colours::grey);
    variantBox.addItem("Classic",1);variantBox.addItem("Compact",2);variantBox.addItem("Glass",3);variantBox.addItem("Terminal",4);variantBox.setSelectedId((int)proc.uiVariant()+1,juce::dontSendNotification);
    for(int i=0;i<10;i++)gridBox.addItem(juce::String(i+1)+"  "+kyoto::gridName((kyoto::GridStyle)i),i+1);gridBox.setSelectedId((int)proc.uiLayout().grid+1,juce::dontSendNotification);
    for(int i=0;i<5;i++)screenBox.addItem(juce::String(i+1)+"  "+kyoto::screenName((kyoto::WaveScreenType)i),i+1);screenBox.setSelectedId(1,juce::dontSendNotification);
    canvas=std::make_unique<BuilderCanvas>(*this);addAndMakeVisible(*canvas); canvas->setVisible(false);
    moduleModel=std::make_unique<ModuleModel>(*this);modules.setModel(moduleModel.get());communityModel=std::make_unique<CommunityModel>(*this);communityList.setModel(communityModel.get());
    expert.setToggleState(proc.isExpertMode(),juce::dontSendNotification);
    populateThemes();refreshFeed();refreshCommunity();syncBuilderFromProcessor();startTimer(12000);show(Home);
}
KyotoSpxritEditor::~KyotoSpxritEditor(){stopTimer();}
void KyotoSpxritEditor::style(juce::Button&b){b.setColour(juce::TextButton::buttonColourId,proc.theme().panel);b.setColour(juce::TextButton::textColourOffId,proc.theme().accent);b.setColour(juce::TextButton::buttonOnColourId,proc.theme().accentDim);}
void KyotoSpxritEditor::label(juce::Label&l,const juce::String&s,float size){l.setText(s,juce::dontSendNotification);l.setFont(size);l.setColour(juce::Label::textColourId,proc.theme().text);}
void KyotoSpxritEditor::paint(juce::Graphics&g){auto&th=proc.theme();g.fillAll(th.bg);g.setColour(th.panel);g.fillRect(0,0,getWidth(),58);g.setColour(th.accent);g.fillRect(0,56,getWidth(),2);g.setColour(th.accent.withAlpha(th.glowAlpha));g.fillEllipse(-80,-120,420,300);g.fillEllipse(getWidth()-340,getHeight()-160,420,300);}
void KyotoSpxritEditor::show(Page p){if(!proc.loggedIn()&&p!=Home)p=Home;page=p;bool h=p==Home,s=p==Selector,b=p==Builder,c=p==Community,t=p==Themes;home.setVisible(!h);selector.setVisible(!s);builder.setVisible(!b);community.setVisible(!c);themesPage.setVisible(!t);selector.setEnabled(proc.loggedIn());builder.setEnabled(proc.loggedIn());community.setEnabled(proc.loggedIn());themesPage.setEnabled(proc.loggedIn());login.setVisible(!proc.loggedIn()&&h);logout.setVisible(proc.loggedIn()&&h);update.setVisible(h);user.setVisible(!proc.loggedIn()&&h);pass.setVisible(!proc.loggedIn()&&h);threads.setVisible(h&&proc.loggedIn());postText.setVisible(h&&proc.loggedIn());post.setVisible(h&&proc.loggedIn());postWav.setVisible(h&&proc.loggedIn());postImage.setVisible(h&&proc.loggedIn());refresh.setVisible(h&&proc.loggedIn());modules.setVisible(s);lockLabel.setVisible(s&&!proc.hasModules());moduleName.setVisible(b);saveName.setVisible(b);saveModule.setVisible(b);uploadCommunity.setVisible(b&&!isFxBuild());expert.setVisible(b);fxViewport.setVisible(b);addFx.setVisible(b&&proc.isExpertMode());for(auto&sld:{&macroTone,&macroPunch,&macroSpace,&macroMovement,&macroWidth})sld->setVisible(b&&!proc.isExpertMode());for(auto*sld:{&oscMix1,&oscMix2,&oscMix3,&detune2,&detune3,&cutoff,&resonance,&attack,&decay,&sustain,&release,&noise,&drive,&lfoRate,&lfoDepth,&octave})sld->setVisible(b&&proc.isExpertMode()&&!isFxBuild());canvas->setVisible(b);addDial.setVisible(b);addSlider.setVisible(b);addScreen.setVisible(b);addText.setVisible(b);deleteElement.setVisible(b);gridBox.setVisible(b);screenBox.setVisible(b);elementBox.setVisible(b);elementText.setVisible(b);showGrid.setVisible(b);themeHint.setVisible(b);screenHint.setVisible(b);communityList.setVisible(c&&proc.loggedIn());loadCommunity.setVisible(c&&proc.loggedIn());themeBox.setVisible(t&&proc.loggedIn());variantBox.setVisible(t&&proc.loggedIn());themeName.setVisible(t&&proc.loggedIn());themeAccent.setVisible(t&&proc.loggedIn());makeTheme.setVisible(t&&proc.loggedIn());resized();rebuildFxRows();repaint();}
void KyotoSpxritEditor::refreshFeed(){if(!proc.loggedIn())return;feed.clear();proc.api().feed(feed,online);if(!threadModel)threadModel=std::make_unique<FeedModel>(*this);threads.setModel(threadModel.get());threads.updateContent();status.setText("DREAMSHARE  •  "+juce::String(online)+" ONLINE",juce::dontSendNotification);}
void KyotoSpxritEditor::doLogin(){auto s=proc.api().login(user.getText().trim(),pass.getText());if(s.ok){proc.setSession(s);user.clear();pass.clear();status.setText("LOGGED IN AS "+proc.user(),juce::dontSendNotification);ensureCustomBuilderTheme();show(Home);refreshFeed();}else status.setText("LOGIN FAILED: "+s.error,juce::dontSendNotification);}
void KyotoSpxritEditor::postPlain(){if(proc.api().postText(proc.token(),postText.getText(),"DreamShare")){postText.clear();refreshFeed();}}
void KyotoSpxritEditor::postFile(bool image){auto chooser=std::make_shared<juce::FileChooser>(image?"Choose an image":"Choose a WAV",juce::File{},image?"*.png;*.jpg;*.jpeg;*.gif":"*.wav");chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this,chooser,image](const juce::FileChooser&c){auto f=c.getResult();if(f.existsAsFile()){bool ok=image?proc.api().postImage(proc.token(),f,postText.getText()):proc.api().postAudio(proc.token(),f,postText.getText());status.setText(ok?"POSTED TO DREAMSHARE":"POST FAILED — CHECK WORKER",juce::dontSendNotification);postText.clear();refreshFeed();}});}
void KyotoSpxritEditor::syncBuilderFromProcessor()
{
    auto p = proc.getInstrumentPreset();

    macroTone.setValue(p.cutoff, juce::dontSendNotification);
    macroPunch.setValue(p.drive, juce::dontSendNotification);
    macroSpace.setValue(
        juce::jlimit(0.0, 1.0, (p.release - .05) / 3.0),
        juce::dontSendNotification);
    macroMovement.setValue(p.lfoDepth, juce::dontSendNotification);
    macroWidth.setValue(
        juce::jlimit(0.0, 1.0, (p.detune2 + 50.0) / 100.0),
        juce::dontSendNotification);

    oscMix1.setValue(p.mix1, juce::dontSendNotification);
    oscMix2.setValue(p.mix2, juce::dontSendNotification);
    oscMix3.setValue(p.mix3, juce::dontSendNotification);
    detune2.setValue(p.detune2, juce::dontSendNotification);
    detune3.setValue(p.detune3, juce::dontSendNotification);
    cutoff.setValue(p.cutoff, juce::dontSendNotification);
    resonance.setValue(p.resonance, juce::dontSendNotification);
    attack.setValue(p.attack, juce::dontSendNotification);
    decay.setValue(p.decay, juce::dontSendNotification);
    sustain.setValue(p.sustain, juce::dontSendNotification);
    release.setValue(p.release, juce::dontSendNotification);
    noise.setValue(p.noise, juce::dontSendNotification);
    drive.setValue(p.drive, juce::dontSendNotification);
    lfoRate.setValue(p.lfoRate, juce::dontSendNotification);
    lfoDepth.setValue(p.lfoDepth, juce::dontSendNotification);
    octave.setValue(p.octave, juce::dontSendNotification);

    proc.setUiLayout(
        p.uiLayout.isVoid()
            ? kyoto::defaultLayout()
            : kyoto::layoutFromVar(p.uiLayout));

    elementBox.clear();

    const auto& layout = proc.uiLayout();

    for (const auto& element : layout.elements)
        elementBox.addItem(
            element.label.isEmpty()
                ? element.id
                : element.label,
            elementBox.getNumItems() + 1);

    if (!layout.elements.empty())
        elementBox.setSelectedId(
            1,
            juce::dontSendNotification);

    rebuildFxRows();
    populateThemes();

    if (canvas)
        canvas->repaint();
}
void KyotoSpxritEditor::syncProcessorFromBuilder(){auto p=proc.getInstrumentPreset();if(proc.isExpertMode()){p.mix1=oscMix1.getValue();p.mix2=oscMix2.getValue();p.mix3=oscMix3.getValue();p.detune2=detune2.getValue();p.detune3=detune3.getValue();p.cutoff=cutoff.getValue();p.resonance=resonance.getValue();p.attack=attack.getValue();p.decay=decay.getValue();p.sustain=sustain.getValue();p.release=release.getValue();p.noise=noise.getValue();p.drive=drive.getValue();p.lfoRate=lfoRate.getValue();p.lfoDepth=lfoDepth.getValue();p.octave=(int)octave.getValue();}else if(isFxBuild()){auto c=proc.fxChain();if(c.empty())for(int i=0;i<8;i++)proc.addFxSlot(i);if(!proc.fxChain().empty()){auto f=proc.fxChain()[0];f.amount=macroTone.getValue();f.tone=macroPunch.getValue();f.motion=macroMovement.getValue();f.mix=macroWidth.getValue();f.shape=macroSpace.getValue();proc.setFxChainSlot(0,f);}}else{p.cutoff=macroTone.getValue();p.drive=macroPunch.getValue();p.release=.05f+(float)macroSpace.getValue()*3.f;p.lfoDepth=macroMovement.getValue();p.detune2=-50.f+(float)macroWidth.getValue()*100.f;p.detune3=-p.detune2;}if(!isFxBuild())proc.setInstrumentPreset(p);}
void KyotoSpxritEditor::saveBuilder()
{
    ensureCustomBuilderTheme();
    syncProcessorFromBuilder();

    auto name = saveName.getText().trim();

    if (name.isEmpty())
    {
        status.setText(
            isFxBuild()
                ? "ENTER AN EFFECT NAME ABOVE"
                : "ENTER AN INSTRUMENT NAME ABOVE",
            juce::dontSendNotification);
        return;
    }

    proc.saveModule(name);

    status.setText(
        "SAVED: " + name
            + " — CUSTOM UI/THEME EMBEDDED",
        juce::dontSendNotification);

    show(Selector);
}
void KyotoSpxritEditor::uploadCurrent()
{
    if (!proc.loggedIn())
    {
        status.setText(
            "LOGIN REQUIRED TO UPLOAD",
            juce::dontSendNotification);
        return;
    }

    if (isFxBuild())
    {
        status.setText(
            "COMMUNITY PUBLISHING IS CURRENTLY FOR INSTRUMENTS",
            juce::dontSendNotification);
        return;
    }

    ensureCustomBuilderTheme();
    syncProcessorFromBuilder();

    kyoto::CommunityInstrument item;

    item.name =
        proc.activeModule().isNotEmpty()
            ? proc.activeModule()
            : "Kyoto Instrument";

    item.description =
        postText.getText().trim();

    if (item.description.isEmpty())
        item.description = "Made in KyotoSpxrit";

    auto p = proc.getInstrumentPreset();

    p.uiLayout =
        kyoto::layoutToVar(proc.uiLayout());

    p.uiTheme =
        kyoto::ThemeManager::toVar(proc.theme());

    item.state =
        kyoto::presetToVar(p);

    const bool ok =
        proc.api().communityPublish(
            proc.token(),
            item);

    status.setText(
        ok
            ? "UPLOADED TO COMMUNITY INSTRUMENTS"
            : "COMMUNITY UPLOAD FAILED — CHECK DREAMSHARE_KV",
        juce::dontSendNotification);

    if (ok)
        refreshCommunity();
}
void KyotoSpxritEditor::refreshCommunity(){if(!proc.loggedIn())return;communityItems.clear();if(proc.api().communityList(communityItems)){if(!communityModel)communityModel=std::make_unique<CommunityModel>(*this);communityList.setModel(communityModel.get());communityList.updateContent();}}
void KyotoSpxritEditor::loadSelectedCommunity(){int r=gSelectedCommunityRow;if(r<0||r>=(int)communityItems.size())r=communityList.getSelectedRow();if(r<0||r>=(int)communityItems.size())return;kyoto::CommunityInstrument item;if(proc.api().communityGet(communityItems[(size_t)r].id,item)){kyoto::InstrumentPreset p;if(kyoto::presetFromVar(item.state,p)){proc.setInstrumentPreset(p);if(!p.uiTheme.isVoid()){auto ct=kyoto::ThemeManager::fromVar(p.uiTheme,proc.theme());ct.custom=true;proc.addCustomTheme(ct);}syncBuilderFromProcessor();status.setText("COMMUNITY INSTRUMENT LOADED: "+item.name,juce::dontSendNotification);show(Builder);}}}
void KyotoSpxritEditor::checkUpdate(){auto info=kyoto::UpdateService().check(KyotoSpxritProcessor::version);status.setText(info.available?"MODULE UPDATE AVAILABLE: "+info.version:"KYOTO'S DREAM IS UP TO DATE",juce::dontSendNotification);}
void KyotoSpxritEditor::populateThemes(){themeBox.clear();int id=1;for(auto&t:proc.themes())themeBox.addItem(t.name+" — "+t.tag,id++);for(auto&t:proc.customThemes())themeBox.addItem("★ "+t.name,id++);int ix=1;for(int i=0;i<(int)proc.themes().size();i++)if(proc.themes()[(size_t)i].id==proc.themeId())ix=i+1;themeBox.setSelectedId(ix,juce::dontSendNotification);}
void KyotoSpxritEditor::applyThemeSelection(){int n=themeBox.getSelectedItemIndex();if(n<0)return;if(n<(int)proc.themes().size()){proc.setTheme(proc.themes()[(size_t)n].id);if(proc.loggedIn())proc.api().setTheme(proc.token(),proc.themeId());}else{int c=n-(int)proc.themes().size();if(c>=0&&c<(int)proc.customThemes().size())proc.setTheme(proc.customThemes()[(size_t)c].id);}for(auto*b:{&home,&selector,&builder,&community,&themesPage,&login,&logout,&update,&refresh,&post,&postWav,&postImage,&saveModule,&uploadCommunity,&loadCommunity,&addFx,&addDial,&addSlider,&addScreen,&addText,&deleteElement,&makeTheme})style(*b);if(canvas)canvas->repaint();repaint();}
void KyotoSpxritEditor::createTheme(){auto name=themeName.getText().trim();auto accent=kyoto::ThemeManager::colour(themeAccent.getText(),proc.theme().accent);auto custom=proc.theme();custom.id="custom-"+juce::String(juce::Time::getCurrentTime().toMilliseconds());custom.name=name.isEmpty()?"My Dream Theme":name;custom.tag="user-created";custom.accent=accent;custom.accentBright=accent.brighter(.4f);custom.accentDim=accent.darker(.5f);custom.accent2=accent.withRotatedHue(.15f);custom.custom=true;proc.addCustomTheme(custom);populateThemes();status.setText("CUSTOM THEME CREATED — UI ELEMENT LAYOUT IS EMBEDDED WITH MODULES",juce::dontSendNotification);}
void KyotoSpxritEditor::applyVariant(){proc.setUiVariant((kyoto::UiVariant)juce::jlimit(0,3,variantBox.getSelectedId()-1));repaint();}
void KyotoSpxritEditor::setExpert(bool b){proc.setExpertMode(b);syncFxFromProcessor();show(Builder);status.setText(b?"EXPERT MODE — UNLIMITED FX CHAIN + FULL PRECISION":"NORMAL MODE — 8 FX SLOTS + SIMPLE MACROS",juce::dontSendNotification);}
void KyotoSpxritEditor::rebuildFxRows(){if(page!=Builder)return;fxRows.clear();fxContent.reset();auto c=proc.fxChain();if(c.empty()){for(int i=0;i<8;i++)proc.addFxSlot(i);c=proc.fxChain();}size_t count=proc.isExpertMode()?c.size():std::min<size_t>(8,c.size());fxContent=std::make_unique<juce::Component>();fxContent->setSize(780,(int)count*48+8);for(size_t i=0;i<count;i++){auto*row=new FxRow(*this,i);fxContent->addAndMakeVisible(row);row->setBounds(4,(int)i*48,772,44);row->sync();fxRows.push_back(row);}fxViewport.setViewedComponent(fxContent.get(),false);fxContent->setVisible(true);}
void KyotoSpxritEditor::syncFxFromProcessor(){rebuildFxRows();}
void KyotoSpxritEditor::addFxToChain(){if(!proc.isExpertMode())return;proc.addFxSlot(0);rebuildFxRows();status.setText("FX ADDED — EXPERT CHAIN IS UNLIMITED",juce::dontSendNotification);}
void KyotoSpxritEditor::addElement(kyoto::BuilderElementType type){auto l=proc.uiLayout();kyoto::UiElement e;e.type=type;e.id="element-"+juce::String(l.elements.size()+1);e.label=type==kyoto::BuilderElementType::Dial?"Dial":type==kyoto::BuilderElementType::Slider?"Slider":type==kyoto::BuilderElementType::WaveScreen?"Wave Shape":"Text";e.x=.05f+(l.elements.size()%4)*.22f;e.y=.08f+(l.elements.size()/4)*.16f;if(type==kyoto::BuilderElementType::WaveScreen)e.screen=(kyoto::WaveScreenType)juce::jlimit(0,4,screenBox.getSelectedId()-1);if(type==kyoto::BuilderElementType::Text)e.text="Your text";l.elements.push_back(e);proc.setUiLayout(l);elementBox.addItem(e.label,elementBox.getNumItems()+1);elementBox.setSelectedId(elementBox.getNumItems(),juce::dontSendNotification);if(canvas)canvas->repaint();}
void KyotoSpxritEditor::removeElement()
{
    int i = elementBox.getSelectedItemIndex();
    auto l = proc.uiLayout();

    if (i < 0 || i >= (int) l.elements.size())
        return;

    l.elements.erase(
        l.elements.begin() + i);

    proc.setUiLayout(l);

    elementBox.clear();

    for (const auto& e : l.elements)
        elementBox.addItem(
            e.label.isEmpty() ? e.id : e.label,
            elementBox.getNumItems() + 1);

    if (!l.elements.empty())
        elementBox.setSelectedId(
            juce::jlimit(
                1,
                elementBox.getNumItems(),
                i + 1),
            juce::dontSendNotification);

    if (canvas)
        canvas->repaint();
}
void KyotoSpxritEditor::selectElement(int i)
{
    auto l = proc.uiLayout();

    if (i < 0 || i >= (int) l.elements.size())
        return;

    elementBox.setSelectedId(
        i + 1,
        juce::dontSendNotification);

    auto& e = l.elements[(size_t) i];

    elementText.setText(
        e.text,
        juce::dontSendNotification);

    screenBox.setSelectedId(
        (int) e.screen + 1,
        juce::dontSendNotification);
}
void KyotoSpxritEditor::elementChanged(){int i=elementBox.getSelectedItemIndex();if(i<0)return;auto l=proc.uiLayout();if(i<(int)l.elements.size()){elementText.setText(l.elements[(size_t)i].text,juce::dontSendNotification);screenBox.setSelectedId((int)l.elements[(size_t)i].screen+1,juce::dontSendNotification);}}
void KyotoSpxritEditor::updateElementText(){int i=elementBox.getSelectedItemIndex();auto l=proc.uiLayout();if(i>=0&&i<(int)l.elements.size()&&l.elements[(size_t)i].type==kyoto::BuilderElementType::Text){l.elements[(size_t)i].text=elementText.getText();proc.setUiLayout(l);if(canvas)canvas->repaint();}}
void KyotoSpxritEditor::applyGrid(){auto l=proc.uiLayout();l.grid=(kyoto::GridStyle)juce::jlimit(0,9,gridBox.getSelectedId()-1);proc.setUiLayout(l);if(canvas)canvas->repaint();}
void KyotoSpxritEditor::applyScreen(){int i=elementBox.getSelectedItemIndex();auto l=proc.uiLayout();if(i>=0&&i<(int)l.elements.size()&&l.elements[(size_t)i].type==kyoto::BuilderElementType::WaveScreen){l.elements[(size_t)i].screen=(kyoto::WaveScreenType)juce::jlimit(0,4,screenBox.getSelectedId()-1);proc.setUiLayout(l);if(canvas)canvas->repaint();}}
void KyotoSpxritEditor::ensureCustomBuilderTheme(){if(!proc.theme().custom){auto t=proc.theme();t.id="module-ui-"+juce::String(juce::Time::getCurrentTime().toMilliseconds());t.name=(proc.activeModule().isEmpty()?"Kyoto Module":proc.activeModule())+" UI";t.tag="module-custom-ui";t.custom=true;proc.addCustomTheme(t);populateThemes();}auto l=proc.uiLayout();l.custom=true;if(l.elements.empty())l=kyoto::defaultLayout();proc.setUiLayout(l);}
void KyotoSpxritEditor::timerCallback(){if(page==Home&&proc.loggedIn())refreshFeed();}
void KyotoSpxritEditor::resized(){auto a=getLocalBounds().reduced(14);auto top=a.removeFromTop(42);title.setBounds(15,8,250,38);status.setBounds(270,8,getWidth()-560,38);modeLabel.setBounds(getWidth()-285,8,160,38);home.setBounds(top.removeFromRight(64).reduced(2));selector.setBounds(top.removeFromRight(78).reduced(2));builder.setBounds(top.removeFromRight(68).reduced(2));community.setBounds(top.removeFromRight(92).reduced(2));themesPage.setBounds(top.removeFromRight(70).reduced(2));login.setBounds(getWidth()/2-160,290,320,42);user.setBounds(getWidth()/2-160,210,320,36);pass.setBounds(getWidth()/2-160,255,320,36);logout.setBounds(getWidth()-110,62,90,28);update.setBounds(15,62,130,28);refresh.setBounds(155,62,90,28);threads.setBounds(30,110,getWidth()-60,getHeight()-250);postText.setBounds(30,getHeight()-125,getWidth()-310,75);post.setBounds(getWidth()-270,getHeight()-125,70,30);postWav.setBounds(getWidth()-190,getHeight()-125,85,30);postImage.setBounds(getWidth()-95,getHeight()-125,85,30);lockLabel.setBounds(240,270,700,40);modules.setBounds(30,120,360,getHeight()-180);communityList.setBounds(30,120,getWidth()-60,getHeight()-180);loadCommunity.setBounds(410,getHeight()-70,160,35);themeBox.setBounds(50,140,500,34);variantBox.setBounds(570,140,220,34);themeName.setBounds(50,200,300,34);themeAccent.setBounds(370,200,240,34);makeTheme.setBounds(630,200,250,34);moduleName.setBounds(30,88,430,30);saveName.setBounds(30,125,260,34);saveModule.setBounds(300,125,150,34);uploadCommunity.setBounds(460,125,180,34);expert.setBounds(30,170,180,30);addFx.setBounds(220,170,120,30);themeHint.setBounds(30,205,700,24);addDial.setBounds(30,240,95,30);addSlider.setBounds(130,240,105,30);addScreen.setBounds(240,240,145,30);addText.setBounds(390,240,100,30);deleteElement.setBounds(495,240,135,30);elementBox.setBounds(30,278,220,32);gridBox.setBounds(260,278,190,32);screenHint.setBounds(460,278,110,18);screenBox.setBounds(565,278,210,32);elementText.setBounds(785,278,250,32);showGrid.setBounds(1040,278,130,32);canvas->setBounds(30,320,745,getHeight()-350);fxViewport.setBounds(800,205,getWidth()-830,getHeight()-235);int by=620;for(auto*s:{&macroTone,&macroPunch,&macroSpace,&macroMovement,&macroWidth})s->setBounds(800,by+=32,getWidth()-850,28);int ex=520;int col=0;for(auto*s:{&oscMix1,&oscMix2,&oscMix3,&detune2,&detune3,&cutoff,&resonance,&attack,&decay,&sustain,&release,&noise,&drive,&lfoRate,&lfoDepth,&octave}){s->setBounds(800+(col%2)*180,ex+(col/2)*34,165,28);++col;} }
juce::String KyotoSpxritEditor::elementTypeName(kyoto::BuilderElementType t)const{return t==kyoto::BuilderElementType::Dial?"Dial":t==kyoto::BuilderElementType::Slider?"Slider":t==kyoto::BuilderElementType::WaveScreen?"Wave Screen":"Text";}
